#ifndef OVERLOADED_HH_INCLUDED_
#define OVERLOADED_HH_INCLUDED_

template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

#endif // OVERLOADED_HH_INCLUDED_
