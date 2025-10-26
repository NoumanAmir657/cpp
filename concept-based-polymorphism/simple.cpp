#include <iostream>
#include <vector>
#include <memory>

// 1. The Concept: Defines the virtual interface.
struct AnimalConcept {
    virtual ~AnimalConcept() = default;

    // The pure virtual function that all models must implement.
    virtual void makeNoise() const = 0;
};

// 2. The Model: Adapts a concrete type to the Concept.
template <typename ConcreteAnimal>
struct AnimalModel : public AnimalConcept {
    // The ConcreteAnimal object we are holding.
    ConcreteAnimal animal;

    // Constructor to initialize the held object.
    AnimalModel(ConcreteAnimal a) : animal(std::move(a)) {}

    // Implements the Concept's method by forwarding the call
    // to the concrete object's method.
    void makeNoise() const override {
        animal.makeNoise();
    }
};

// 3. The Wrapper (The Type-Eraser): Holds any type that fits the concept.
class Animal {
private:
    // A smart pointer to hold the concrete model object polymorphically.
    std::unique_ptr<AnimalConcept> pImpl;

public:
    // This is the key constructor that enables polymorphism.
    // It takes ANY type T and wraps it in an AnimalModel.
    template <typename T>
    Animal(T animal) : pImpl(std::make_unique<AnimalModel<T>>(std::move(animal))) {}

    // The public interface method, which delegates the call to the
    // virtual method on the stored concept implementation.
    void makeNoise() const {
        // If the wrapper is empty, we might throw or assert,
        // but for simplicity, we'll assume it's always valid.
        if (pImpl) {
            pImpl->makeNoise();
        }
    }
};

// The concrete types—they don't need to inherit from any Animal base class!
struct Dog {
    void makeNoise() const { std::cout << "Woof! 🐶\n"; }
};

struct Cat {
    void makeNoise() const { std::cout << "Meow. 😼\n"; }
};

int main() {
    // We can put different concrete types into a single container of the wrapper type.
    std::vector<Animal> zoo;

    // The Dog and Cat objects are implicitly converted into Animal objects
    // via the template constructor, which creates the appropriate AnimalModel.
    zoo.emplace_back(Dog{});
    zoo.emplace_back(Cat{});

    // We iterate over the vector and call the makeNoise method polymorphically.
    for (const auto& animal : zoo) {
        animal.makeNoise();
    }
}
