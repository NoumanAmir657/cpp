#include <iostream>
#include <memory>

struct SideEffectsInterfaceTraits {
    struct Concept {
        virtual ~Concept() = default;
        virtual bool hasSideEffect(void* op) const = 0;
    };

    template <typename ConcreteOp>
    struct Model : public Concept {
        bool hasSideEffect(void* op) const override {
            ConcreteOp* concreteOp = static_cast<ConcreteOp*>(op);
            return concreteOp->hasSideEffect();
        }
    };
};

template <typename DerivedInterface, typename Traits>
class BaseInterface {
public:
    class Trait {
    private:
        typename Traits::Concept* vconcept;

    public:
        template <typename ConcreteType>
        void initialize() {
            vconcept = new typename Traits::template Model<ConcreteType>();
        }

        typename Traits::Concept* getConcept() { return vconcept; }

        ~Trait() { delete vconcept; }
    };

protected:
    void* entity;
    typename Traits::Concept* vconcept;

public:
    BaseInterface(void* e, typename Traits::Concept* c)
        : entity(e), vconcept(c) {}

    typename Traits::Concept* getImpl() const { return vconcept; }
    void* getEntity() const { return entity; }
};

class SideEffectsInterface : public BaseInterface<SideEffectsInterface, SideEffectsInterfaceTraits> {
public:
    using BaseInterface<SideEffectsInterface, SideEffectsInterfaceTraits>::BaseInterface;

    bool hasSideEffect() const {
        return getImpl()->hasSideEffect(getEntity());
    }
};

template <typename Derived, typename... Traits>
class TraitManager {
private:
    std::tuple<Traits...> traits;

public:
    TraitManager() {
        initTraits(std::index_sequence_for<Traits...>{});
    }

    template <std::size_t... Is>
    void initTraits(std::index_sequence<Is...>) {
        (std::get<Is>(traits).template initialize<Derived>(), ...);
    }

    template <typename Trait>
    auto getTraitConcept() {
        return std::get<Trait>(traits).getConcept();
    }
};

class AddOp : public TraitManager<AddOp, SideEffectsInterface::Trait> {
public:
    bool hasSideEffect() {
        return false;
    }
};

class SubOp : public TraitManager<SubOp, SideEffectsInterface::Trait> {
public:
    bool hasSideEffect() {
        return false;
    }
};

class LoadOp : public TraitManager<LoadOp, SideEffectsInterface::Trait> {
public:
    bool hasSideEffect() {
        return true;
    }
};

template <typename Interface, typename Derived, typename... Traits>
Interface cast_to_interface(TraitManager<Derived, Traits...>* animal) {
    if constexpr ((std::is_same_v<Traits, typename Interface::Trait> || ...)) {
        auto* vconcept = animal->template getTraitConcept<typename Interface::Trait>();
        return Interface(animal, vconcept);
    } else {
        throw std::runtime_error("Op does not implement interface");
    }
}

int main() {
    AddOp addOp;
    SubOp subOp;
    LoadOp loadOp;

    std::cout << "=== Using the interface ===\n";

    SideEffectsInterface addOpSideEffects = cast_to_interface<SideEffectsInterface>(&addOp);
    std::cout << "Add operation has side effect = " << addOpSideEffects.hasSideEffect() << "\n";

    SideEffectsInterface subOpSideEffects = cast_to_interface<SideEffectsInterface>(&subOp);
    std::cout << "Sub operation has side effect = " << subOpSideEffects.hasSideEffect() << "\n";

    SideEffectsInterface loadOpSideEffects = cast_to_interface<SideEffectsInterface>(&loadOp);
    std::cout << "Load operation has side effect = " << loadOpSideEffects.hasSideEffect() << "\n";

    return 0;
}
