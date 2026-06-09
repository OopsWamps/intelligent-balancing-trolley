/* Stubs for inv_mpu.c and inv_mpu_dmp_motion_driver.c functions.
 * Saves ~12KB by removing unused DMP code when using Kalman/complementary filter. */

#define INV_XYZ_GYRO  (0x70)
#define INV_XYZ_ACCEL (0x38)
#define DEFAULT_MPU_HZ 100

void dmp_load_motion_driver_firmware(void) {}
void dmp_set_orientation(unsigned short o) { (void)o; }
void dmp_set_fifo_rate(unsigned short r) { (void)r; }
void dmp_enable_feature(unsigned short m) { (void)m; }
void dmp_set_accel_bias(long *b) { (void)b; }
void dmp_set_gyro_bias(long *b) { (void)b; }
int dmp_read_fifo(short *g, short *a, long *q, unsigned long *t,
                   short *s, unsigned char *m) {
    (void)g;(void)a;(void)q;(void)t;(void)s;(void)m; return -1;
}
int mpu_init(void) { return 0; }
int mpu_set_sensors(unsigned char s) { (void)s; return 0; }
int mpu_configure_fifo(unsigned char s) { (void)s; return 0; }
int mpu_set_sample_rate(unsigned short r) { (void)r; return 0; }
int mpu_set_int_config(unsigned char i) { (void)i; return 0; }
int mpu_set_int_latched(unsigned char l) { (void)l; return 0; }
int mpu_set_dmp_state(unsigned char e) { (void)e; return 0; }
int mpu_get_gyro_sens(unsigned short *s) { *s = 0; return 0; }
int mpu_get_accel_sens(unsigned short *s) { *s = 0; return 0; }
int mpu_set_lpf(unsigned short a) { (void)a; return 0; }
int mpu_run_self_test(long *g, long *a) { (void)g;(void)a; return 0; }
unsigned short inv_orientation_matrix_to_scalar(const signed char *m) {
    (void)m; return 0;
}
signed char gyro_orientation[9] = {0};
