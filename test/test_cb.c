#include "unity.h"
#include "cb.h"


void test_basics(void) {
  char xval;
  circular_buffer buffer;
  cb_init(&buffer, 200);

  char* test = "Hello world";
  for (int i = 0; i < sizeof(test); i++) {
    cb_push_back(&buffer, test+i);
  }

  TEST_ASSERT_TRUE(buffer.count == sizeof(test));

  for (int i = 0; i < sizeof(test); i++) {
    char charval;
    int retval = cb_pop_front(&buffer, &charval);
    TEST_ASSERT_TRUE(retval == 0);
    TEST_ASSERT_TRUE(test[i] == charval);
  }

  TEST_ASSERT_TRUE(cb_pop_front(&buffer, &xval) == -1);
}

void test_simple_circle(void) {
  char xval;
  circular_buffer buffer;
  cb_init(&buffer, 200);

  for (int i = 0; i < 5000; i++) {
    for (int j = 0; j < 17; j++) {
      char inval = 'A' + j;
      TEST_ASSERT_FALSE(cb_push_back(&buffer, &inval));
    }

    // printf("buffer=%p buffer_end=%p capacity=%d count=%d head=%p tail=%p\n",
    //   buffer.buffer, buffer.buffer_end, buffer.capacity,
    //   buffer.count, buffer.head, buffer.tail);

    for (int j = 0; j < 17; j++) {
      char retval;
      TEST_ASSERT_FALSE(cb_pop_front(&buffer, &retval));
      TEST_ASSERT_TRUE((char)('A' + j) == retval);
    }

    TEST_ASSERT_TRUE(cb_pop_front(&buffer, &xval) == -1);
  }
}

void test_overflow(void) {
  circular_buffer buffer;
  cb_init(&buffer, 42);

  for (int i = 0; i < 5000; i++) {
    for (int j = 0; j < 50; j++) {
      char inval = 'A' + j;
      int retval = cb_push_back(&buffer, &inval);
      TEST_ASSERT_TRUE(retval == (j < 42 ? 0 : -1));
    }

    for (int j = 0; j < 50; j++) {
      int retval;
      char retchar;
      retval = cb_pop_front(&buffer, &retchar);
      if (j < 42) {
        TEST_ASSERT_FALSE(retval);
        if ((char)('A' + j) != retchar) {
          char mesg[300];
          snprintf(mesg, 300, "loop=%d, j=%d, retchar=%d\n", i, j, retchar);
          TEST_FAIL_MESSAGE(mesg);
        }
        TEST_ASSERT_TRUE((char)('A' + j) == retchar);
      } else {
        TEST_ASSERT_TRUE(retval == -1);
      }
    }

    TEST_ASSERT_TRUE(buffer.count == 0);
  }
}

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_basics);
  RUN_TEST(test_simple_circle);
  RUN_TEST(test_overflow);
  return UNITY_END();
}


/**
  * For native dev-platform or for some embedded frameworks
  */
int main(void) {
  return runUnityTests();
}

/**
  * For ESP-IDF framework
  */
void app_main() {
  runUnityTests();
}