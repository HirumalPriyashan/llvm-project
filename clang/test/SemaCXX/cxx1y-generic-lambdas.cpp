// RUN: %clang_cc1 -std=c++1y -verify -fsyntax-only -fblocks -emit-llvm -o - %s
// DONTRUNYET: %clang_cc1 -std=c++1y -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// DONTRUNYET: %clang_cc1 -std=c++1y -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// DONTRUNYET: %clang_cc1 -std=c++1y -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING

namespace explicit_call {
int test() {
  auto L = [](auto a) { return a; };
  L.operator()(3);
  L.operator()<char>(3.14); //expected-warning{{implicit conversion}}
  return 0;
}  
} //end ns

namespace test_conversion_to_fptr {

void f1(int (*)(int)) { }
void f2(char (*)(int)) { } // expected-note{{candidate}}
void g(int (*)(int)) { } // #1 expected-note{{candidate}}
void g(char (*)(char)) { } // #2 expected-note{{candidate}}
void h(int (*)(int)) { } // #3
void h(char (*)(int)) { } // #4

int test() {
{
  auto glambda = [](auto a) { return a; };
  glambda(1);
  f1(glambda); // OK
  f2(glambda); // expected-error{{no matching function}}
  g(glambda); // expected-error{{call to 'g' is ambiguous}}
  h(glambda); // OK: calls #3 since it is convertible from ID
  
  int& (*fpi)(int*) = [](auto* a) -> auto& { return *a; }; // OK
  
}
{
  
  auto L = [](auto a) { return a; };
  int (*fp)(int) = L;
  fp(5);
  L(3);
  char (*fc)(char) = L;
  fc('b');
  L('c');
  double (*fd)(double) = L;
  fd(3.14);
  fd(6.26);
  L(4.25);
}
{
  auto L = [](auto a) ->int { return a; }; //expected-note 2{{candidate template ignored}}
  int (*fp)(int) = L;
  char (*fc)(char) = L; //expected-error{{no viable conversion}}
  double (*fd)(double) = L; //expected-error{{no viable conversion}}
}

}

namespace more_converion_to_ptr_to_function_tests {


int test() {
  {
    int& (*fpi)(int*) = [](auto* a) -> auto& { return *a; }; // OK
    int (*fp2)(int) = [](auto b) -> int {  return b; };
    int (*fp3)(char) = [](auto c) -> int { return c; };
    char (*fp4)(int) = [](auto d) { return d; }; //expected-error{{no viable conversion}}\
                                                 //expected-note{{candidate template ignored}}
    char (*fp5)(char) = [](auto e) -> int { return e; }; //expected-error{{no viable conversion}}\
                                                 //expected-note{{candidate template ignored}}

    fp2(3);
    fp3('\n');
    fp3('a');
    return 0;
  }
} // end test()

template<class ... Ts> void vfun(Ts ... ) { }

int variadic_test() {

 int (*fp)(int, char, double) = [](auto ... a) -> int { vfun(a...); return 4; };
 fp(3, '4', 3.14);
 
 int (*fp2)(int, char, double) = [](auto ... a) { vfun(a...); return 4; };
 fp(3, '4', 3.14);
 return 2;
}

} // end ns

namespace conversion_operator {
void test() {
    auto L = [](auto a) -> int { return a; };
    int (*fp)(int) = L; 
    int (&fp2)(int) = [](auto a) { return a; };  // expected-error{{non-const lvalue}}
    int (&&fp3)(int) = [](auto a) { return a; };  // expected-error{{no viable conversion}}\
                                                  //expected-note{{candidate}}
  }
}
}

namespace return_type_deduction_ok {
 auto l = [](auto a) ->auto { return a; }(2); 
 auto l2 = [](auto a) ->decltype(auto) { return a; }(2);  
 auto l3 = [](auto a) { return a; }(2); 

}

namespace generic_lambda_as_default_argument_ok {
  void test(int i = [](auto a)->int { return a; }(3)) {
  }
}

namespace nested_non_capturing_lambda_tests {
template<class ... Ts> void print(Ts ...) { }
int test() {
{
  auto L = [](auto a) {
    return [](auto b) {
      return b;
    };
  };
  auto M = L(3);
  M(4.15);
 }
{
  int i = 10; //expected-note{{declared here}}
  auto L = [](auto a) {
    return [](auto b) { //expected-note{{begins here}}
      i = b;  //expected-error{{cannot be implicitly captured}}
      return b;
    };
  };
  auto M = L(3);
  M(4.15); //expected-note{{instantiation}}
 }
 {
  auto L = [](auto a) {
    print("a = ", a, "\n");
    return [](auto b) ->decltype(a) {
      print("b = ", b, "\n");
      return b;
    };
  };
  auto M = L(3);
  M(4.15);
 }
 
{
  auto L = [](auto a) ->decltype(a) {
    print("a = ", a, "\n");
    return [](auto b) ->decltype(a) { //expected-error{{no viable conversion}}\
                                      //expected-note{{candidate template ignored}}
      print("b = ", b, "\n");
      return b;
    };
  };
  auto M = L(3); //expected-note{{in instantiation of}}
 }
{
  auto L = [](auto a) {
    print("a = ", a, "\n");
    return [](auto ... b) ->decltype(a) {
      print("b = ", b ..., "\n");
      return 4;
    };
  };
  auto M = L(3);
  M(4.15, 3, "fv");
}

{
  auto L = [](auto a) {
    print("a = ", a, "\n");
    return [](auto ... b) ->decltype(a) {
      print("b = ", b ..., "\n");
      return 4;
    };
  };
  auto M = L(3);
  int (*fp)(double, int, const char*) = M; 
  fp(4.15, 3, "fv");
}

{
  auto L = [](auto a) {
    print("a = ", a, "\n");
    return [](char b) {
      return [](auto ... c) ->decltype(b) {
        print("c = ", c ..., "\n");
        return 42;
      };
    };
  };
  L(4);
  auto M = L(3);
  M('a');
  auto N = M('x');
  N("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
  char (*np)(const char*, int, const char*, double, const char*, int) = N;
  np("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
}


{
  auto L = [](auto a) {
    print("a = ", a, "\n");
    return [](decltype(a) b) {
      return [](auto ... c) ->decltype(b) {
        print("c = ", c ..., "\n");
        return 42;
      };
    };
  };
  L('4');
  auto M = L('3');
  M('a');
  auto N = M('x');
  N("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
  char (*np)(const char*, int, const char*, double, const char*, int) = N;
  np("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
}


{
 struct X {
  static void foo(double d) { } 
  void test() {
    auto L = [](auto a) {
      print("a = ", a, "\n");
      foo(a);
      return [](decltype(a) b) {
        foo(b);
        foo(sizeof(a) + sizeof(b));
        return [](auto ... c) ->decltype(b) {
          print("c = ", c ..., "\n");
          foo(decltype(b){});
          foo(sizeof(decltype(a)*) + sizeof(decltype(b)*));
          return 42;
        };
      };
    };
    L('4');
    auto M = L('3');
    M('a');
    auto N = M('x');
    N("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
    char (*np)(const char*, int, const char*, double, const char*, int) = N;
    np("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
  }
};
X x;
x.test();
}
// Make sure we can escape the function
{
 struct X {
  static void foo(double d) { } 
  auto test() {
    auto L = [](auto a) {
      print("a = ", a, "\n");
      foo(a);
      return [](decltype(a) b) {
        foo(b);
        foo(sizeof(a) + sizeof(b));
        return [](auto ... c) ->decltype(b) {
          print("c = ", c ..., "\n");
          foo(decltype(b){});
          foo(sizeof(decltype(a)*) + sizeof(decltype(b)*));
          return 42;
        };
      };
    };
    return L;
  }
};
  X x;
  auto L = x.test();
  L('4');
  auto M = L('3');
  M('a');
  auto N = M('x');
  N("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
  char (*np)(const char*, int, const char*, double, const char*, int) = N;
  np("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
}

{
 struct X {
  static void foo(double d) { } 
  auto test() {
    auto L = [](auto a) {
      print("a = ", a, "\n");
      foo(a);
      return [](decltype(a) b) {
        foo(b);
        foo(sizeof(a) + sizeof(b));
        return [](auto ... c) {
          print("c = ", c ..., "\n");
          foo(decltype(b){});
          foo(sizeof(decltype(a)*) + sizeof(decltype(b)*));
          return [](decltype(c) ... d) ->decltype(a) { //expected-note{{candidate}}
            print("d = ", d ..., "\n");
            foo(decltype(b){});
            foo(sizeof(decltype(a)*) + sizeof(decltype(b)*));
            return decltype(a){};
          };
        };
      };
    };
    return L;
  }
};
  X x;
  auto L = x.test();
  L('4');
  auto M = L('3');
  M('a');
  auto N = M('x');
  auto O = N("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
  char (*np)(const char*, int, const char*, double, const char*, int) = O;
  np("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
  int (*np2)(const char*, int, const char*, double, const char*, int) = O; // expected-error{{no viable conversion}}
  
}
} // end test()

namespace wrapped_within_templates {

namespace explicit_return {
template<class T> int fooT(T t) {
  auto L = [](auto a) -> void { 
    auto M = [](char b) -> void {
      auto N = [](auto c) -> void {
        int x = 0;
        x = sizeof(a);        
        x = sizeof(b);
        x = sizeof(c);
      };  
      N('a');
      N(decltype(a){});
    };    
  };
  L(t);
  L(3.14);
  return 0;
}

int run = fooT('a') + fooT(3.14);

} // end explicit_return

namespace implicit_return_deduction {
template<class T> auto fooT(T t) {
  auto L = [](auto a)  { 
    auto M = [](char b)  {
      auto N = [](auto c)  {
        int x = 0;
        x = sizeof(a);        
        x = sizeof(b);
        x = sizeof(c);
      };  
      N('a');
      N(decltype(a){});
    };    
  };
  L(t);
  L(3.14);
  return 0;
}

int run = fooT('a') + fooT(3.14);

template<class ... Ts> void print(Ts ... ts) { }

template<class F, class ... Rest> using first = F;

template<class ... Ts> auto fooV(Ts ... ts) {
  auto L = [](auto ... a) { 
    auto M = [](decltype(a) ... b) {  
      auto N = [](auto c) {
        int x = 0;
        x = sizeof...(a);        
        x = sizeof...(b);
        x = sizeof(c);
      };  
      N('a');
      N(N);
      N(first<Ts...>{});
    };
    M(a...);
    print("a = ", a..., "\n");    
  };
  L(L, ts...);
  print("ts = ", ts..., "\n");
  return 0;
}

int run2 = fooV(3.14, " ", '4', 5) + fooV("BC", 3, 2.77, 'A', float{}, short{}, unsigned{});

} //implicit_return_deduction


} //wrapped_within_templates

namespace at_ns_scope {
  void foo(double d) { }
  auto test() {
    auto L = [](auto a) {
      print("a = ", a, "\n");
      foo(a);
      return [](decltype(a) b) {
        foo(b);
        foo(sizeof(a) + sizeof(b));
        return [](auto ... c) {
          print("c = ", c ..., "\n");
          foo(decltype(b){});
          foo(sizeof(decltype(a)*) + sizeof(decltype(b)*));
          return [](decltype(c) ... d) ->decltype(a) { //expected-note{{candidate}}
            print("d = ", d ..., "\n");
            foo(decltype(b){});
            foo(sizeof(decltype(a)*) + sizeof(decltype(b)*));
            return decltype(a){};
          };
        };
      };
    };
    return L;
  }
auto L = test();
auto L_test = L('4');
auto M = L('3');
auto M_test = M('a');
auto N = M('x');
auto O = N("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
char (*np)(const char*, int, const char*, double, const char*, int) = O;
auto NP_result = np("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
int (*np2)(const char*, int, const char*, double, const char*, int) = O; // expected-error{{no viable conversion}}



} 

namespace variadic_tests_1 {
template<class ... Ts> void print(Ts ... ts) { }

template<class F, class ... Rest> using FirstType = F;
template<class F, class ... Rest> F& FirstArg(F& f, Rest...) { return f; }
 
template<class ... Ts> int fooV(Ts ... ts) {
  auto L = [](auto ... a) -> void { 
    auto M = [](decltype(a) ... b) -> void {  
      auto N = [](auto c) -> void {
        int x = 0;
        x = sizeof...(a);        
        x = sizeof...(b);
        x = sizeof(c);
      };  
      N('a');
      N(N);
      N(FirstType<Ts...>{});
    };
    M(a...);
    print("a = ", a..., "\n");    
  };
  L(L, ts...);
  print("ts = ", ts..., "\n");
  return 0;
}

int run2 = fooV(3.14, " ", '4', 5) + fooV("BC", 3, 2.77, 'A', float{}, short{}, unsigned{});

namespace more_variadic_1 {

template<class ... Ts> int fooV(Ts ... ts) {
  auto L = [](auto ... a) { 
    auto M = [](decltype(a) ... b) -> void {  
      auto N = [](auto c) -> void {
        int x = 0;
        x = sizeof...(a);        
        x = sizeof...(b);
        x = sizeof(c);
      };  
      N('a');
      N(N);
      N(FirstType<Ts...>{});
    };
    M(a...);
    return M;
  };
  auto M = L(L, ts...);
  decltype(L(L, ts...)) (*fp)(decltype(L), decltype(ts) ...) = L;
  void (*fp2)(decltype(L), decltype(ts) ...) = L(L, ts...);
  
  {
    auto L = [](auto ... a) { 
      auto M = [](decltype(a) ... b) {  
        auto N = [](auto c) -> void {
          int x = 0;
          x = sizeof...(a);        
          x = sizeof...(b);
          x = sizeof(c);
        };  
        N('a');
        N(N);
        N(FirstType<Ts...>{});
        return N;
      };
      M(a...);
      return M;
    };
    auto M = L(L, ts...);
    decltype(L(L, ts...)) (*fp)(decltype(L), decltype(ts) ...) = L;
    fp(L, ts...);
    decltype(L(L, ts...)(L, ts...)) (*fp2)(decltype(L), decltype(ts) ...) = L(L, ts...);
    fp2 = fp(L, ts...);
    void (*fp3)(char) = fp2(L, ts...);
    fp3('a');
  }
  return 0;
}

int run2 = fooV(3.14, " ", '4', 5) + fooV("BC", 3, 2.77, 'A', float{}, short{}, unsigned{});


} //end ns more_variadic_1

} // end ns variadic_tests_1

namespace at_ns_scope_within_class_member {
 struct X {
  static void foo(double d) { } 
  auto test() {
    auto L = [](auto a) {
      print("a = ", a, "\n");
      foo(a);
      return [](decltype(a) b) {
        foo(b);
        foo(sizeof(a) + sizeof(b));
        return [](auto ... c) {
          print("c = ", c ..., "\n");
          foo(decltype(b){});
          foo(sizeof(decltype(a)*) + sizeof(decltype(b)*));
          return [](decltype(c) ... d) ->decltype(a) { //expected-note{{candidate}}
            print("d = ", d ..., "\n");
            foo(decltype(b){});
            foo(sizeof(decltype(a)*) + sizeof(decltype(b)*));
            return decltype(a){};
          };
        };
      };
    };
    return L;
  }
};
X x;
auto L = x.test();
auto L_test = L('4');
auto M = L('3');
auto M_test = M('a');
auto N = M('x');
auto O = N("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
char (*np)(const char*, int, const char*, double, const char*, int) = O;
auto NP_result = np("\n3 = ", 3, "\n6.14 = ", 6.14, "\n4'123'456 = ", 4'123'456);
int (*np2)(const char*, int, const char*, double, const char*, int) = O; // expected-error{{no viable conversion}}
  
} //end at_ns_scope_within_class_member


namespace nested_generic_lambdas_123 {
void test() {
  auto L = [](auto a) -> int {
    auto M = [](auto b, decltype(a) b2) -> int { 
      return 1;
    };
    M(a, a);
  };
  L(3); 
}
template<class T> void foo(T) {
 auto L = [](auto a) { return a; }; 
}
template void foo(int); 
} // end ns nested_generic_lambdas_123


} // end ns nested_non_capturing_lambda_tests
 