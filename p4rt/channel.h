#ifndef CHANNEL_H_
#define CHANNEL_H_


#include <list>
#include <thread>
#include <condition_variable>
template<class item>
class Channel {
private:
  std::list<item> queue_;
  std::mutex m_;
  std::condition_variable cv_;
public:
  void put(const item &i) {
    std::unique_lock<std::mutex> lock(m_);
    queue_.push_back(i);
    cv_.notify_one();
  }
  item get() {
    std::unique_lock<std::mutex> lock(m_);
    cv_.wait(lock, [&](){ return !queue_.empty(); });
    item result = queue_.front();
    queue_.pop_front();
    return result;
  }
};
#endif