#ifndef TEST_INCLUDED_2FE25057_6E39_44A2_95A3_5C1C16ED162B
#define TEST_INCLUDED_2FE25057_6E39_44A2_95A3_5C1C16ED162B


#ifdef __cplusplus
extern "C" {
#endif


void *test_device();
int test_setup(struct drv_app_ctx *ctx);
void test_tick();
void test_shutdown();


#ifdef __cplusplus
} /* extern */
#endif


#endif
