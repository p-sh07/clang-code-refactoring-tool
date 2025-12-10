#include <vector>
#include <string>

class Base_NoVDtor {
public:  
  virtual void method1() = 0;
  virtual void method2() = 0;
  virtual void method3() = 0;
  ~Base_NoVDtor() {}
};   

class Derived : public Base_NoVDtor {  
public:
  void method1();
  void method2();
  void method3() override; 
  virtual ~Derived() {} 
};

void Derived::method1() {}
void Derived::method2() {}
void Derived::method3() {}

class Base_NoDerived {
public:  
  ~Base_NoDerived() {}
};   

class Base2_NoVDtor {
public:  
  ~Base2_NoVDtor() {}
}; 

class Derived2 : Base2_NoVDtor {
public:  
    virtual ~Derived2() {} 
}; 

class MyType {
public:
    int id;
    std::string name;
    MyType(int i, const std::string& n) : id(i), name(n) {}
};

void foo() {  
  std::vector<int> v1;  
  for (const auto x1 : v1) {
  }

  for (const int x2 : v1) {
  }  

  std::vector<MyType> vec = {{1, "obj1"}, {2, "obj2"}, {3, "obj3"}};
  for (const auto& const_ref : vec) {
  }
  
  for (const auto const_no_ref : vec) { 
  }
}  
/**Matcher commands tested in clang-query m ...
 cxxDestructorDecl(isExpansionInMainFile(), isDefinition(), unless(isImplicit()), unless(isVirtual())).bind("nvdtor")
 cxxRecordDecl(isExpansionInMainFile(), hasDirectBase(hasType(cxxRecordDecl(has(cxxDestructorDecl())))))
 cxxMethodDecl(isExpansionInMainFile(), isOverride(), unless(hasAttr('attr::Override'))).bind("no_override")
 cxxForRangeStmt(hasLoopVariable(varDecl(hasType(qualType(isConstQualified())), unless(hasType(referenceType())), unless(hasType(builtinType()))))).bind("no_ref")
*/