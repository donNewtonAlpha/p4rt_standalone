/*
 * Copyright 2020 Google LLC
 * Copyright 2020-present Open Networking Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdn_controller_manager.h"

#include "absl/numeric/int128.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "glog/logging.h"
#include "p4/v1/p4runtime.pb.h"

namespace p4rt_server{
namespace {

std::string PrettyPrintRoleName(const absl::optional<std::string>& name) {
  return (name.has_value()) ? absl::StrCat("'", *name, "'") : "<default>";
}

std::string PrettyPrintElectionId(const absl::optional<absl::uint128>& id) {
  if (id.has_value()) {
    p4::v1::Uint128 p4_id;
    p4_id.set_high(absl::Uint128High64(*id));
    p4_id.set_low(absl::Uint128Low64(*id));
    return absl::StrCat("{ ", p4_id.ShortDebugString(), " }");
  }
  return "<backup>";
}

grpc::Status ValidateConnection(
    const absl::optional<std::string>& role_name,
    const absl::optional<absl::uint128>& election_id,
    const std::vector<SdnConnection*>& active_connections) {
  // If the election ID is not set then the controller is saying this should be
  // a backup connection, and we allow any number of backup connections.
  if (!election_id.has_value()) return grpc::Status::OK;

  // Otherwise, we verify the election ID is unique among all active connections
  // for a given role (including the root role).
  for (const auto& connection : active_connections) {
    if (connection->GetRoleName() == role_name &&
        connection->GetElectionId() == election_id) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Election ID is already used by another connection "
                          "with the same role.");
    }
  }
  return grpc::Status::OK;
}

}  // namespace

void SdnConnection::SetElectionId(const absl::optional<absl::uint128>& id) {
  election_id_ = id;
}

absl::optional<absl::uint128> SdnConnection::GetElectionId() const {
  return election_id_;
}

void SdnConnection::SetRoleName(const absl::optional<std::string>& name) {
  role_name_ = name;
}

absl::optional<std::string> SdnConnection::GetRoleName() const {
  return role_name_;
}

void SdnConnection::SendStreamMessageResponse(
    const p4::v1::StreamMessageResponse& response) {
  if (!grpc_stream_->Write(response)) {
    LOG(ERROR) << "Could not send arbitration update response to gRPC conext '"
               << grpc_context_ << "': " << response.ShortDebugString();
  }
}

grpc::Status SdnControllerManager::HandleArbitrationUpdate(
    const p4::v1::MasterArbitrationUpdate& update, SdnConnection* controller) {
  absl::MutexLock l(&lock_);

  // TODO: arbitration should fail with invalid device id.
  device_id_ = update.device_id();

  // Verify the request's device ID is being sent to the correct device.
  if (update.device_id() != device_id_) {
    return grpc::Status(
        grpc::StatusCode::FAILED_PRECONDITION,
        absl::StrCat("Arbitration request has the wrong device ID '",
                     update.device_id(),
                     "'. Cannot establish connection to this device '",
                     device_id_, "'."));
  }

  // If the role name is not set then we assume the connection is a 'root'
  // connection.
  absl::optional<std::string> role_name;
  if (update.has_role() && !update.role().name().empty()) {
    role_name = update.role().name();
  }

  // If the election ID is not set then we assume the controller does not want
  // this connection to be the primary connection.
  absl::optional<absl::uint128> election_id;
  if (update.has_election_id()) {
    election_id = absl::MakeUint128(update.election_id().high(),
                                    update.election_id().low());
  }

  // If the controller is already initialized we check if the role & election ID
  // match. Assuming nothing has changed then there is nothing we need to do.
  if (controller->IsInitialized() && controller->GetRoleName() == role_name &&
      controller->GetElectionId() == election_id) {
    SendArbitrationResponse(controller);
    return grpc::Status::OK;
  }

  // Verify that this is a valid connection, and wont mess up internal state.
  auto valid_connection =
      ValidateConnection(role_name, election_id, connections_);
  if (!valid_connection.ok()) {
    return valid_connection;
  }

  // Update the connection with the arbitration data and initalize.
  if (controller->IsInitialized()) {
    LOG(INFO) << absl::StreamFormat(
        "Update SDN connection (%s, %s): %s",
        PrettyPrintRoleName(controller->GetRoleName()),
        PrettyPrintElectionId(controller->GetElectionId()),
        update.ShortDebugString());
  } else {
    LOG(INFO) << "New SDN connection: " << update.ShortDebugString();
  }
  controller->SetRoleName(role_name);
  controller->SetElectionId(election_id);
  controller->Initialize();
  connections_.push_back(controller);

   // If there is a change in the primary connection state we should inform all
  // other connections with the same role. Otherwise, we just respond directly
  // to the calling controller.
  if (UpdateToPrimaryConnectionState(role_name, election_id)) {
    InformConnectionsAboutPrimaryChange(role_name);
  } else {
    // primary connection didn't so inform just this connection that it is a
    // backup.
    SendArbitrationResponse(controller);
  }
  // Determine if there is any primary connection state changes. This check will
  // analize all the active connections so it needs to be done BEFORE we update
  // any connection state.
  return grpc::Status::OK;
}

void SdnControllerManager::Disconnect(SdnConnection* connection) {
  absl::MutexLock l(&lock_);

  // If the connection was never initialized then there is no work needed to
  // disconnect it.
  if (!connection->IsInitialized()) return;

  // Iterate through the list connections and remove this connection.
  for (auto iter = connections_.begin(); iter != connections_.end(); ++iter) {
    if (*iter == connection) {
      LOG(INFO) << "Dropping SDN connection for role "
                << PrettyPrintRoleName(connection->GetRoleName())
                << " with election ID "
                << PrettyPrintElectionId(connection->GetElectionId()) << ".";
      connections_.erase(iter);
      break;
    }
  }

  // If connection was the primary connection we need to inform all existing
  // connections.
  if (connection->GetElectionId().has_value() &&
      (connection->GetElectionId() ==
      election_id_past_by_role_[connection->GetRoleName()])) {
    InformConnectionsAboutPrimaryChange(connection->GetRoleName());
  }
}

grpc::Status SdnControllerManager::AllowRequest(
    const absl::optional<std::string>& role_name,
    const absl::optional<absl::uint128>& election_id) {
  absl::MutexLock l(&lock_);

  if (!election_id.has_value()) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "Request does not have an election ID.");
  }

  const auto& primary_election_id = election_id_past_by_role_.find(role_name);
  if (primary_election_id == election_id_past_by_role_.end()) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "Only the primary connection can issue requests, but "
                        "no primary connection has been established.");
  }

  if (election_id != primary_election_id->second) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "Only the primary connection can issue requests.");
  }
  return grpc::Status::OK;
}

grpc::Status SdnControllerManager::AllowRequest(
    const p4::v1::WriteRequest& request) {
  absl::optional<std::string> role_name;
  if (!request.role().empty()) {
    role_name = request.role();
  }

  absl::optional<absl::uint128> election_id;
  if (request.has_election_id()) {
    election_id = absl::MakeUint128(request.election_id().high(),
                                    request.election_id().low());
  }
  return AllowRequest(role_name, election_id);
}

grpc::Status SdnControllerManager::AllowRequest(
    const p4::v1::SetForwardingPipelineConfigRequest& request) {
  absl::optional<std::string> role_name;
  if (!request.role().empty()) {
    role_name = request.role();
  }

  absl::optional<absl::uint128> election_id;
  if (request.has_election_id()) {
    election_id = absl::MakeUint128(request.election_id().high(),
                                    request.election_id().low());
  }
  return AllowRequest(role_name, election_id);
}

bool SdnControllerManager::UpdateToPrimaryConnectionState(
    const absl::optional<std::string>& role_name,
    const absl::optional<absl::uint128>& election_id) {
  VLOG(1) << "Checking for new primary connections.";
  // Find the highest election ID, from the active connections, for the role.
  absl::optional<absl::uint128> max_election_id;
  for (const auto& connection_ptr : connections_) {
    if (connection_ptr->GetRoleName() != role_name) continue;
    max_election_id =
        std::max(max_election_id, connection_ptr->GetElectionId());
  }

  // Get the highest election ID currently seen. This does not need to be from
  // an active connection.
  absl::optional<absl::uint128>& election_id_past =
      election_id_past_by_role_[role_name];

  // If the election_id arugment (i.e. the ID from the update request) equals
  // the election_id_past, then we are in a state where the old primary
  // connection was either disconneded or downgraded to a backup. In this case
  // we need to inform all the active connections.
  bool old_primary_is_reconnecting = (election_id == max_election_id);

  if (max_election_id != election_id_past || old_primary_is_reconnecting) {
    if (max_election_id.has_value() && max_election_id > election_id_past) {
      LOG(INFO) << "New primary connection for role "
                << PrettyPrintRoleName(role_name) << " with election ID "
                << PrettyPrintElectionId(max_election_id) << ".";

      // Only update current election ID if there is a higher value.
      election_id_past = max_election_id;
    } else if (max_election_id.has_value() &&
               max_election_id == election_id_past) {
      LOG(INFO) << "Old primary connection for role "
                << PrettyPrintRoleName(role_name)
                << " is becoming the current primary again with election ID "
                << PrettyPrintElectionId(max_election_id) << ".";
    } else {
      LOG(INFO) << "No longer have a primary connection for role "
                << PrettyPrintRoleName(role_name) << ".";
    }
    return true;
  }
  VLOG(1) << "Primary connection has not changed.";
  return false;
}

void SdnControllerManager::InformConnectionsAboutPrimaryChange(
    const absl::optional<std::string>& role_name) {
  VLOG(1) << "Informing all connections about primary connection change.";
  for (const auto& connection : connections_) {
    if (connection->GetRoleName() == role_name) {
      SendArbitrationResponse(connection);
    }
  }
}

bool SdnControllerManager::PrimaryConnectionExists(
    const absl::optional<std::string>& role_name) {
  absl::optional<absl::uint128> primary_election_id =
     election_id_past_by_role_[role_name];

  for (const auto& connection : connections_) {
    if (connection->GetRoleName() == role_name &&
        connection->GetElectionId() == primary_election_id) {
      return primary_election_id.has_value();
    }
  }
  return false;
}

void SdnControllerManager::SendArbitrationResponse(SdnConnection* connection) {
  p4::v1::StreamMessageResponse response;
  auto arbitration = response.mutable_arbitration();

  // Always set device ID.
  arbitration->set_device_id(device_id_);

  // Populate the role only if the connection has set one.
  if (connection->GetRoleName().has_value()) {
    *arbitration->mutable_role()->mutable_name() =
        connection->GetRoleName().value();
  }

  // Populate the election ID with the highest accepted value.
  absl::optional<absl::uint128> primary_election_id =
      election_id_past_by_role_[connection->GetRoleName()];
  if (primary_election_id.has_value()) {
    arbitration->mutable_election_id()->set_high(
        absl::Uint128High64(primary_election_id.value()));
    arbitration->mutable_election_id()->set_low(
        absl::Uint128Low64(primary_election_id.value()));
  }

  // Update connection status for the arbitration response.
  auto status = arbitration->mutable_status();
  if (PrimaryConnectionExists(connection->GetRoleName())) {
    // has primary connection.
    if (primary_election_id == connection->GetElectionId()) {
      // and this connection is it.
      status->set_code(grpc::StatusCode::OK);
      status->set_message("you are the primary connection.");
    } else {
      // but this connection is a backup.
      status->set_code(grpc::StatusCode::ALREADY_EXISTS);
      status->set_message(
          "you are a backup connection, and a primary connection exists.");
    }
  } else {
    // no primary connection exists.
    status->set_code(grpc::StatusCode::NOT_FOUND);
    status->set_message(
        "you are a backup connection, and NO primary connection exists.");
  }
  connection->SendStreamMessageResponse(response);
}

bool SdnControllerManager::SendStreamMessageToPrimary(
    const absl::optional<std::string>& role_name,
    const p4::v1::StreamMessageResponse& response) {
  absl::MutexLock l(&lock_);

  // Get the primary election ID for the controller role.
  absl::optional<absl::uint128> primary_election_id =
      election_id_past_by_role_[role_name];

  // If there is no election ID set, then there is no primary connection.
  if (!primary_election_id.has_value()) return false;

  // Otherwise find the primary connection.
  SdnConnection* primary_connection = nullptr;
  for (const auto& connection : connections_) {
    if (connection->GetRoleName() == role_name &&
        connection->GetElectionId() == primary_election_id) {
      primary_connection = connection;
    }
  }

  if (primary_connection == nullptr) {
    LOG(ERROR) << "Found an election ID '"
               << PrettyPrintElectionId(primary_election_id)
               << "' for the primary connection, but could not find the "
               << "connection itself?";
    return false;
  }

  primary_connection->SendStreamMessageResponse(response);
  return true;
}

}  // namespace p4rt_app
