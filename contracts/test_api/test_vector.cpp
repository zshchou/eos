#include <eoslib/eos.hpp>
#include <eoslib/types.hpp>
#include <eoslib/vector.hpp>

#include "test_api.hpp"

using namespace eosio;

struct call_register {
  int constructor;
  int copy_constructor;
  int destructor;
  int assignment;
  call_register() { reset(); }
  void reset() {
    constructor = 0;
    copy_constructor = 0;
    destructor = 0;
    assignment = 0;
  }

  bool all_zero() {
    return !(constructor | copy_constructor | destructor | assignment);
  }

  void print() {
    eosio::print("> CG:\n\tc) ", constructor, "\n\td) ", destructor, "\n\tcc) ", copy_constructor, "\n\ta) ", assignment, "\n");
  }

} cg;

class test_struct {
public:
  uint64_t a;
  uint64_t b;
  test_struct() : test_struct(0,0) { }
  test_struct(uint64_t a, uint64_t b) : a(a), b(b) { cg.constructor++; }
  test_struct(const test_struct& rhs) { a=rhs.a; b=rhs.b; /*eosio::print(">after =\n\t");print();*/ cg.copy_constructor++; }
  test_struct& operator=(const test_struct& rhs) { a=rhs.a; b=rhs.b; cg.assignment++; return *this;}
  ~test_struct() { cg.destructor++; }
  void print() {
    eosio::print(a, "-", b, "\n");
  }
};

unsigned int test_vector::construct_all() {

  vector<test_struct> cv1;
  WASM_ASSERT(cv1.get_capacity() == 0, "cv1 capacity");
  WASM_ASSERT(cv1.get_size() == 0, "cv1 size");
  WASM_ASSERT(cv1.get_data() == nullptr, "cv1 data");
  WASM_ASSERT(cv1.get_ref_count() == 0, "cv1 ref_count");

  test_struct tmp[] = {{1,2},{2,3},{4,5}};
  cg.reset();

  vector<test_struct> cv2(tmp, 3);

  WASM_ASSERT(cv2.get_capacity() == 3, "cv2 capacity");
  WASM_ASSERT(cv2.get_size() == 3, "cv2 size");
  WASM_ASSERT(cv2.get_data() == tmp, "cv2 data");
  WASM_ASSERT(cv2.get_ref_count() == 0, "cv2 ref_count");

  for(int i=0; i<3; ++i) {
    WASM_ASSERT(cv2[i].a == tmp[i].a && cv2[i].b == tmp[i].b, "cv2 comp");
  }

  WASM_ASSERT(cg.all_zero() == true, "cv2 zero");
  cv2.clear();

  WASM_ASSERT(cv2.get_capacity() == 0, "cv2 capacity 2");
  WASM_ASSERT(cv2.get_size() == 0, "cv2 size 2");
  WASM_ASSERT(cv2.get_data() == nullptr, "cv2 data 2");
  WASM_ASSERT(cv2.get_ref_count() == 0, "cv2 ref_count 2");
  WASM_ASSERT(cg.all_zero() == true, "cv2 zero 2");

  vector<test_struct> cv3(10);
  cv3.push_back(test_struct(0,1));
  cv3.push_back(test_struct(2,3));
  cv3.push_back(test_struct(4,5));
  cv3.push_back(test_struct(6,7));
  cv3.push_back(test_struct(8,9));

  WASM_ASSERT(cv3.get_capacity() == 10, "cv3 capacity");
  WASM_ASSERT(cv3.get_size() == 5, "cv3 size");
  WASM_ASSERT(cv3.get_data() != nullptr, "cv3 data");
  WASM_ASSERT(cv3.get_ref_count() == 1, "cv3 ref_count");

  vector<test_struct> cv4(cv3);
  WASM_ASSERT(cv4.get_capacity() == cv4.get_capacity(), "cv4 capacity");
  WASM_ASSERT(cv4.get_size() == cv4.get_size(), "cv4 size");
  WASM_ASSERT(cv4.get_data() == cv4.get_data(), "cv4 data");
  WASM_ASSERT(cv4.get_ref_count() == 2, "cv4 ref_count");

  cv3.clear();
  cv4.clear();
  
  WASM_ASSERT(cv3.get_ref_count() == cv4.get_ref_count(), "cv3/cv4 ref_count");
  WASM_ASSERT(cv3.get_ref_count() == 0, "cv3/cv4 ref_count");

  return WASM_TEST_PASS;
}

unsigned int test_vector::assignment_operator() {

  vector<test_struct> ao1;
  ao1.push_back(test_struct(0,1));
  ao1.push_back(test_struct(2,3));
  ao1.push_back(test_struct(4,5));

  WASM_ASSERT(ao1.get_capacity() == 10, "ao1 capacity");
  WASM_ASSERT(ao1.get_size() == 3, "ao1 size");
  WASM_ASSERT(ao1.get_data() != nullptr, "ao1 data");
  WASM_ASSERT(ao1.get_ref_count() == 1, "ao1 ref_count");

  vector<test_struct> ao2;
  ao2 = ao1;

  WASM_ASSERT(ao2.get_capacity() == 10, "ao2 capacity");
  WASM_ASSERT(ao2.get_size() == 3, "ao2 size");
  WASM_ASSERT(ao2.get_data() != nullptr, "ao2 data");
  WASM_ASSERT(ao2.get_ref_count() == 2, "ao2 ref_count");

  WASM_ASSERT(ao2.get_capacity() == ao1.get_capacity(), "ao2/ao1 capacity");
  WASM_ASSERT(ao2.get_size() == ao1.get_size(), "ao2/ao1 size");
  WASM_ASSERT(ao2.get_data() == ao1.get_data(), "ao2/ao1 data");
  WASM_ASSERT(ao2.get_ref_count() == ao1.get_ref_count(), "ao2/ao1 ref_count");

  return WASM_TEST_PASS;
}

unsigned int test_vector::reserve() {

  vector<test_struct> v1;
  WASM_ASSERT(v1.get_capacity() == 0, "v1 capacity 1");
  WASM_ASSERT(v1.get_size() == 0, "v1 size 1");
  WASM_ASSERT(v1.get_data() == nullptr, "v1 data 1");
  WASM_ASSERT(v1.get_ref_count() == 0, "v1 ref_count 1");

  v1.reserve(5);
  WASM_ASSERT(v1.get_capacity() == 5, "v1 capacity 2");
  WASM_ASSERT(v1.get_size() == 0, "v1 size 2");
  WASM_ASSERT(v1.get_data() != nullptr, "v1 data 2");
  WASM_ASSERT(v1.get_ref_count() == 1, "v1 ref_count 2");

  v1.reserve(2);
  WASM_ASSERT(v1.get_capacity() == 5, "v1 capacity 3");
  WASM_ASSERT(v1.get_size() == 0, "v1 size 3");
  WASM_ASSERT(v1.get_data() != nullptr, "v1 data 3");
  WASM_ASSERT(v1.get_ref_count() == 1, "v1 ref_count 3");

  v1.push_back(test_struct(0,1));
  v1.push_back(test_struct(2,3));
  v1.push_back(test_struct(4,5));
  v1.push_back(test_struct(6,7));
  v1.push_back(test_struct(8,9));

  WASM_ASSERT(v1.get_capacity() == 5, "v1 capacity 4");
  WASM_ASSERT(v1.get_size() == 5, "v1 size 4");
  WASM_ASSERT(v1.get_data() != nullptr, "v1 data 4");
  WASM_ASSERT(v1.get_ref_count() == 1, "v1 ref_count 4");

  WASM_ASSERT(cg.copy_constructor == 5, "v1 cc");
  cg.reset();

  vector<test_struct> v2 = v1;
  WASM_ASSERT(v1.get_ref_count() == 2, "v1 ref_count 5");
  WASM_ASSERT(v2.get_ref_count() == 2, "v2 ref_count 5");

  v2.push_back(test_struct(10,11));
  WASM_ASSERT(v2.get_capacity() == 10, "v2 capacity 5");
  WASM_ASSERT(v2.get_size() == 6, "v2 capacity 5");
  WASM_ASSERT(cg.copy_constructor == 6, "v2 cc");

  WASM_ASSERT(v1.get_ref_count() == 1, "v1 ref_count 6");
  WASM_ASSERT(v2.get_ref_count() == 1, "v2 ref_count 6");

  cg.reset();
  v1.clear();
  WASM_ASSERT(v1.get_ref_count() == 0, "v1 ref_count 6");
  WASM_ASSERT(cg.destructor == 5, "v1 destructor");

  cg.reset();
  v2.clear();
  WASM_ASSERT(v2.get_ref_count() == 0, "v2 ref_count 7");
  WASM_ASSERT(cg.destructor == 6, "v2 destructor");

  WASM_ASSERT(v1.get_capacity() == 0, "v1 capacity 8");
  WASM_ASSERT(v1.get_size() == 0, "v1 size 8");
  WASM_ASSERT(v1.get_data() == nullptr, "v1 data 8");
  WASM_ASSERT(v1.get_ref_count() == 0, "v1 ref_count 8");

  WASM_ASSERT(v2.get_capacity() == 0, "v2 capacity 8");
  WASM_ASSERT(v2.get_size() == 0, "v2 size 8");
  WASM_ASSERT(v2.get_data() == nullptr, "v2 data 8");
  WASM_ASSERT(v2.get_ref_count() == 0, "v2 ref_count 8");

  return WASM_TEST_PASS;
}

unsigned int test_vector::index_operator() {
  vector<test_struct> io1;
  io1.push_back(test_struct(0,1));
  io1.push_back(test_struct(2,3));
  io1.push_back(test_struct(4,5));

  WASM_ASSERT(io1.get_capacity() == 10, "io1 capacity");
  WASM_ASSERT(io1.get_size() == 3, "io1 size");
  WASM_ASSERT(io1.get_data() != nullptr, "io1 data");
  WASM_ASSERT(io1.get_ref_count() == 1, "io1 ref_count");

  cg.reset();
  WASM_ASSERT(io1[0].a == 0 && io1[0].b == 1, "io1[0]");
  WASM_ASSERT(io1[1].a == 2 && io1[1].b == 3, "io1[1]");
  WASM_ASSERT(io1[2].a == 4 && io1[2].b == 5, "io1[2]");
  WASM_ASSERT(cg.all_zero(), "io1 zero");

  return WASM_TEST_PASS;
}

unsigned int test_vector::index_out_of_bound() {
  vector<test_struct> io2;
  io2.push_back(test_struct(0,1));
  io2.push_back(test_struct(2,3));
  io2.push_back(test_struct(4,5));
  io2.push_back(test_struct(6,7));

  WASM_ASSERT(io2.get_capacity() == 10, "io2 capacity");
  WASM_ASSERT(io2.get_size() == 4, "io2 size");
  WASM_ASSERT(io2.get_data() != nullptr, "io2 data");
  WASM_ASSERT(io2.get_ref_count() == 1, "io2 ref_count");

  cg.reset();
  WASM_ASSERT(io2[0].a == 0 && io2[0].b == 1, "io2[0]");
  WASM_ASSERT(io2[1].a == 2 && io2[1].b == 3, "io2[1]");
  WASM_ASSERT(io2[2].a == 4 && io2[2].b == 5, "io2[2]");
  WASM_ASSERT(io2[3].a == 6 && io2[3].b == 7, "io2[3]");
  WASM_ASSERT(cg.all_zero(), "io2 zero");

  auto& tmp = io2[4];

  return WASM_TEST_PASS;
}

unsigned int test_vector::destructor() {
  return WASM_TEST_FAIL;
}

unsigned int test_vector::pod_type_general() {
  return WASM_TEST_FAIL;
}
