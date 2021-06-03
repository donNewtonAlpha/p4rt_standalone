#ifndef GOOGLE_GUTIL_OVERLOADED_H_
#define GOOGLE_GUTIL_OVERLOADED_H_

namespace gutil {

// Useful in conjunction with {std,absl}::visit.
// See https://en.cppreference.com/w/cpp/utility/variant/visit.
template <class... Ts>
struct Overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;

}  // namespace gutil

#endif  // GOOGLE_GUTIL_OVERLOADED_H_
