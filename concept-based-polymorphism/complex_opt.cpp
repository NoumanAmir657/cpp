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

template <typename ConcreteView>
std::optional<ConcreteView> dyn_cast_view(Operation* op) {
  if (op->name == ConcreteView::name) {
    return ConcreteView(op);
  }
  return std::nullopt;
}

int main() {
  // Views on stack, Storage on heap
  auto add1 = createOp<AddOp>(10, 20);
  auto load1 = createOp<LoadOp>(0xA000);
  auto add2 = createOp<AddOp>(30, 40);

  // We store the raw pointers. This is a "Type Erased" list.
  std::vector<Operation*> block;

  block.push_back(add1.getOperation());
  block.push_back(load1.getOperation());
  block.push_back(add2.getOperation());

  std::cout << "=== Iterating over Generic Operations ===\n";

  for (Operation* rawOp : block) {
    std::cout << "Processing op: " << rawOp->name << " -> ";

    if (auto addView = dyn_cast_view<AddOp>(rawOp)) {
      std::cout << "Math: " << addView->getLHS() << " + " << addView->getRHS();
    }
    else if (auto loadView = dyn_cast_view<LoadOp>(rawOp)) {
      std::cout << "Memory Access: " << std::hex << loadView->getAddress();
    }
    else {
      std::cout << "Unknown Op";
    }
    std::cout << "\n";
  }

  // Cleanup
  for (Operation* op : block) delete op;

  return 0;
}
