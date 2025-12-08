#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <stdexcept>
#include <optional>

struct Operation {
  std::string name;
  std::vector<int> operands;
};

template <typename Derived, typename... Traits> class Op {
protected:
  Operation* state;

public:
  explicit Op(Operation* state) : state(state) {}

  Operation* getOperation() const { return state; }

  template <typename T>
  static constexpr bool has_trait = (std::is_same_v<T, Traits> || ...);
};

struct SideEffectsInterfaceTraits {
  struct Concept {
    virtual ~Concept() = default;
    virtual bool hasSideEffect(void *opaqueOp) const = 0;
  };

  template <typename ConcreteOp> struct Model : public Concept {
    bool hasSideEffect(void *opaqueOp) const override {
      Operation* opState = static_cast<Operation*>(opaqueOp);
      ConcreteOp view(opState);
      return view.hasSideEffect();
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


class AddOp : public Op<AddOp, SideEffectsInterfaceTraits> {
public:
  using Op::Op;
  static constexpr const char* name = "addOp";

  int getLHS() const { return state->operands[0]; }
  int getRHS() const { return state->operands[1]; }

  bool hasSideEffect() { return false; }
};

class LoadOp : public Op<LoadOp, SideEffectsInterfaceTraits> {
public:
  using Op::Op;
  static constexpr const char* name = "loadOp";

  int getAddress() const { return state->operands[0]; }

  bool hasSideEffect() { return true; }
};


template <typename ConcreteOp, typename... Args>
ConcreteOp createOp(Args... args) {
  Operation* op = new Operation();
  op->name = ConcreteOp::name;

  (op->operands.push_back(args), ...);

  return ConcreteOp(op);
}

template <typename Interface, typename Derived, typename... Traits>
Interface cast(Op<Derived, Traits...> op) {
  using RequiredTrait = typename Interface::Trait;
  constexpr bool has_trait = Op<Derived, Traits...>::template has_trait<RequiredTrait>;

  static_assert(has_trait, "Op does not implement interface");

  if constexpr (has_trait) {
    static typename RequiredTrait::template Model<Derived> static_model;
    return Interface(op.getOperation(), &static_model);
  } else {
    return Interface(nullptr, nullptr);
  }
}

template <typename Interface, typename Derived, typename... Traits>
std::optional<Interface> dyn_cast(Op<Derived, Traits...> op) {
  using RequiredTrait = typename Interface::Trait;

  if constexpr (Op<Derived, Traits...>::template has_trait<RequiredTrait>) {
    static typename RequiredTrait::template Model<Derived> static_model;
    return Interface(op.getOperation(), &static_model);
  } else {
    return std::nullopt;
  }
}

int main() {
  auto addOp = createOp<AddOp>(10, 20);

  auto loadOp = createOp<LoadOp>(123456789);

  std::cout << "=== Inspecting Data through Views ===\n";
  std::cout << "AddOp: " << addOp.getLHS() << " + " << addOp.getRHS() << "\n";
  std::cout << "LoadOp Addr: " << loadOp.getAddress() << "\n";

  std::cout << "\n=== Using Interfaces ===\n";

  SideEffectsInterface iface1 = cast<SideEffectsInterface>(addOp);
  std::cout << "Add SideEffect: " << iface1.hasSideEffect() << "\n";

  SideEffectsInterface iface2 = cast<SideEffectsInterface>(loadOp);
  std::cout << "Load SideEffect: " << iface2.hasSideEffect() << "\n";

  delete addOp.getOperation();
  delete loadOp.getOperation();

  return 0;
}
