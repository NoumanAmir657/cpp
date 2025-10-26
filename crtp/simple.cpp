#include <iostream>
#include <vector>

template <typename Derived>
class AnimalBase {
public:
    // The public entry point for the "noise" interface.
    // It is NOT virtual, so the call is resolved at compile time.
    void makeNoise() const {
        // This is the core of CRTP: cast 'this' to the derived type
        // and call the method expected on the derived class.
        static_cast<const Derived*>(this)->makeNoiseImpl();
    }
};

class Dog : public AnimalBase<Dog> {
public:
    // This is the specific implementation the CRTP base calls.
    void makeNoiseImpl() const {
        std::cout << "Dog says Woof!\n";
    }
};

class Cat : public AnimalBase<Cat> {
public:
    // This is the specific implementation the CRTP base calls.
    void makeNoiseImpl() const {
        std::cout << "Cat says Meow!\n";
    }
};

// This function is the site of polymorphism.
// It accepts any type T that inherits from AnimalBase<T>.
template <typename T>
void communicate(const AnimalBase<T>& animal) {
    // Calls the base class's non-virtual public method.
    // The compiler knows exactly which derived method will be executed
    // at the moment of compilation.
    animal.makeNoise();
}

int main() {
    Dog myDog;
    Cat myCat;

    std::cout << "--- Communicating via Static Polymorphism (CRTP) ---\n";

    // Call 1: Compiler instantiates communicate(AnimalBase<Dog>&)
    communicate(myDog);

    // Call 2: Compiler instantiates communicate(AnimalBase<Cat>&)
    communicate(myCat);

    // --- CRTP Limitation ---
    // ERROR: This is where CRTP fails for runtime polymorphism.
    // Dog and Cat have different, unrelated base classes (AnimalBase<Dog> and AnimalBase<Cat>).
    // The compiler will reject this attempt to store heterogeneous types.
    // std::vector<AnimalBase<?>> animals = { myDog, myCat };
}
