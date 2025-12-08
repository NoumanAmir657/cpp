#include <iostream>
#include <tuple>
#include <stdexcept>
#include <optional>

struct SideEffectsInterfaceTraits {
  struct Concept {
    virtual ~Concept() = default;
    virtual bool hasSideEffect(void *op) const = 0;
  };

  template <typename ConcreteOp> struct Model : public Concept {
    bool hasSideEffect(void *op) const override {
      ConcreteOp *concreteOp = static_cast<ConcreteOp *>(op);
      return concreteOp->hasSideEffect();
    }
  };
};

template <typename DerivedInterface, typename TraitsType> class BaseInterface {
protected:
  void *entity;
  typename TraitsType::Concept *vconcept;

public:
  using Trait = TraitsType;

  BaseInterface(void *e, typename TraitsType::Concept *c)
      : entity(e), vconcept(c) {}

  typename TraitsType::Concept *getImpl() const { return vconcept; }
  void *getEntity() const { return entity; }
};

class SideEffectsInterface
    : public BaseInterface<SideEffectsInterface, SideEffectsInterfaceTraits> {
public:
  using BaseInterface<SideEffectsInterface,
                      SideEffectsInterfaceTraits>::BaseInterface;

  bool hasSideEffect() const { return getImpl()->hasSideEffect(getEntity()); }
};

template <typename Derived, typename... Traits> class Op {
public:
  template <typename T>
  static constexpr bool has_trait = (std::is_same_v<T, Traits> || ...);
};

class AddOp : public Op<AddOp> {
public:
  bool hasSideEffect() { return false; }
};

class LoadOp : public Op<LoadOp, SideEffectsInterfaceTraits> {
public:
  bool hasSideEffect() { return true; }
};

template <typename Interface, typename Derived, typename... Traits>
Interface cast(Op<Derived, Traits...> *op) {
  using RequiredTrait = typename Interface::Trait;
  constexpr bool implements_trait = Op<Derived, Traits...>::template has_trait<RequiredTrait>;

  static_assert(implements_trait, "\n\n ERROR: You are trying to cast an Op to an Interface it does not implement! \n\n");

  if constexpr (implements_trait) {
    static typename RequiredTrait::template Model<Derived> static_model;
    return Interface(op, &static_model);
  } else {
    std::terminate();
  }
}

template <typename Interface, typename Derived, typename... Traits>
std::optional<Interface> dyn_cast(Op<Derived, Traits...> *op) {
  using RequiredTrait = typename Interface::Trait;

  if constexpr (Op<Derived, Traits...>::template has_trait<RequiredTrait>) {
    static typename RequiredTrait::template Model<Derived> static_model;
    return Interface(op, &static_model);
  } else {
    return std::nullopt;
  }
}

int main() {
  AddOp addOp;
  LoadOp loadOp;

  std::optional<SideEffectsInterface> addOpSideEffects = dyn_cast<SideEffectsInterface>(&addOp);
  if (addOpSideEffects) {
    std::cout << "Add operation has side effect = " << addOpSideEffects->hasSideEffect() << "\n";
  }

  SideEffectsInterface loadOpSideEffects = cast<SideEffectsInterface>(&loadOp);
  std::cout << "Load operation has side effect = " << loadOpSideEffects.hasSideEffect() << "\n";

  return 0;
}
