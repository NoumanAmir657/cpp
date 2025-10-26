#include <iostream>
#include <memory>
#include <vector>
#include <type_traits> // For std::is_same_v

// --- 1. EXTERNAL, UNMODIFIABLE TYPES ---

// These classes come from an external library.
// They do NOT share a common base class.
// They use 'render()' instead of our desired 'draw()'.
struct ExternalCircle {
    void render() const { std::cout << "Rendering a Circle (External).\n"; }
};

struct ExternalSquare {
    void render() const { std::cout << "Rendering a Square (External).\n"; }
};

// --- 2. THE INTERFACE DEFINITION (CONCEPT + TRAITS) ---

// Concept (Abstract Base Class/virtual interface)
struct DrawableConcept {
    virtual ~DrawableConcept() = default;
    virtual void draw() const = 0;
};

// The Traits Class (Holds the Model/Adapter Logic)
struct DrawableTraits {
    // Generic Model (The default adapter for types that might conform perfectly)
    // By conforming, we mean they have a 'draw()' method.
    template <typename T>
    struct Model : public DrawableConcept {
        T item;
        Model(T t) : item(std::move(t)) {}

        // This assumes T has a 'draw()' method.
        // We typically don't use this generic model if methods are mismatched,
        // but it's often used for internal types that do conform.
        void draw() const override { item.draw(); }
    };

    // Explicit Specialization 1: Model for ExternalCircle
    // This Model adapts the external type to our Concept.
    template <>
    struct Model<ExternalCircle> : public DrawableConcept {
        ExternalCircle circle;
        Model(ExternalCircle c) : circle(std::move(c)) {}

        // Implementation of the Concept method:
        // It calls the externally named method 'render()'.
        void draw() const override {
            circle.render();
        }
    };

    // Explicit Specialization 2: Model for ExternalSquare
    // This Model adapts the external type to our Concept.
    template <>
    struct Model<ExternalSquare> : public DrawableConcept {
        ExternalSquare square;
        Model(ExternalSquare s) : square(std::move(s)) {}

        // Implementation of the Concept method:
        // It calls the externally named method 'render()'.
        void draw() const override {
            square.render();
        }
    };
};

// --- 3. THE TYPE-ERASING WRAPPER (THE PUBLIC INTERFACE) ---

// This is the public class that users interact with.
class Drawable {
private:
    std::unique_ptr<DrawableConcept> pImpl;

public:
    // The key templated constructor
    template <typename T>
    Drawable(T item) {
        // The compiler automatically selects the correct specialized
        // 'DrawableTraits::Model<T>' at compile time.
        // This is a single, clean line for all concrete types.
        pImpl = std::make_unique<DrawableTraits::Model<T>>(std::move(item));
    }

    // The public interface function, which delegates to the virtual method.
    void draw() const {
        if (pImpl) pImpl->draw();
    }
};

// --- 4. A CONFORMING INTERNAL TYPE ---

// An internal class that conforms perfectly to the 'draw' name.
// NOTE: For simplicity, we are assuming the Generic Model is NOT used for external types
// due to the name mismatch. In a real scenario, you'd use C++20 Concepts
// to constrain the generic model to only types that have 'draw()'.
struct InternalTriangle {
    void draw() const { std::cout << "Drawing a Triangle (Internal, Conforming).\n"; }
};

int main() {
    ExternalCircle c;
    ExternalSquare s;
    InternalTriangle t;

    std::vector<Drawable> new_scene;
    new_scene.emplace_back(ExternalCircle{});
    new_scene.emplace_back(ExternalSquare{});
    new_scene.emplace_back(InternalTriangle{});

    std::cout << "--- Drawing Scene ---\n";
    for (const auto& item : new_scene) {
        item.draw();
    }

    std::cout << "--- Success ---\n";

    return 0;
}
