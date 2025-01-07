#include <Rhandon.tech-project-1_inferencing.h>  // Edge Impulse library for machine learning model

// Define camera model for configuration
#define CAMERA_MODEL_AI_THINKER

// Required camera libraries
#include "img_converters.h"
#include "image_util.h"
#include "esp_camera.h"
#include "camera_pins.h"

// Global variables for image processing
dl_matrix3du_t *resized_matrix = NULL;
ei_impulse_result_t result = {0};

// Hardware Pin Definitions
#define BTN 4           // Button input pin
#define RX_PIN 12      // Serial1 RX (connects to Arduino Uno TX Pin 9)
#define TX_PIN 13      // Serial1 TX (connects to Arduino Uno RX Pin 8)
#define RELAY_PIN 14   // Controls Arduino relay switch

void setup() {
  // Initialize relay control for Arduino Uno Making Sure its turn off if ESP32-CAM is booting or turn off accidentaly
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);    // Initial relay state OFF
  delay(2000);                     // 2-second startup delay
  digitalWrite(RELAY_PIN, HIGH);   // Set relay ON after delay
  
  // Initialize Serial communications
  Serial.begin(115200);            // Debug serial port
  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);  // Arduino communication port

  // Configure button input
  pinMode(BTN, INPUT);  // Button pin as input

  // Camera configuration structure
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;    // 20MHz clock frequency
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;          // Lower value = higher quality
  config.fb_count = 1;               // Number of frame buffers

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  // Additional camera settings for OV3660 sensor
  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // Flip image vertically
    s->set_brightness(s, 1);    // Increase brightness
    s->set_saturation(s, 0);    // Reduce saturation
  }

  Serial.println("Camera Ready!");
}

void loop() {
  // Wait for button press
  while (!digitalRead(BTN));
  delay(50);  // Debounce delay

  // Read and process incoming data from Arduino
  String result = Serial1.readString();
  result.trim();  // Clean up the string
  result.replace("\r", "");
  result.replace("\n", "");

  // Debug output
  Serial.println("Received: '" + result + "'");
  Serial.print("Length of received: ");
  Serial.println(result.length());

  // State machine for system operation
  if (result == "") {
    // Arduino ready state
    Serial.print("Arduino is ready");
    String result = classify();  // Run image classification
    Serial.printf("[INFO] Result: %s\n", result.c_str());
    Serial1.println(result);     // Send result to Arduino      
  } 
  else if (result == "done_charging") {
    // Post-charging state
    Serial.print("Arduino is not charging");
    String result = classify();
    Serial.printf("[INFO] Result: %s\n", result.c_str());
    Serial1.println(result);
  } 
  else {
    // Charging in progress state
    Serial.print("Arduino is charging");
  }
}

/**
 * Performs image classification using Edge Impulse model
 * Returns classification result as a string
 */
String classify() {
  capture_quick();  // Clear camera buffer

  // Capture and process image
  if (!capture()) return "[ERROR] Capture failed";

  Serial.println("[INFO] Image captured. Running classifier...");

  // Prepare signal for classification
  signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_WIDTH;
  signal.get_data = &raw_feature_get_data;

  // Run Edge Impulse classifier
  Serial.println("[INFO] Running classifier...");
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
  dl_matrix3du_free(resized_matrix);

  if (res != 0) {
    Serial.printf("[ERROR] Classifier error: %d\n", res);
    return "[ERROR] Classifier error";
  }

  // Debug timing information
  ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);

  // Find highest scoring classification
  int index = 0;
  float score = 0.0;
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    if (result.classification[ix].value > score) {
      score = result.classification[ix].value;
      index = ix;
    }
    Serial.printf("[INFO] Label: %s, Score: %f\n", result.classification[ix].label, result.classification[ix].value);
  }

  // Output anomaly score if available
#if EI_CLASSIFIER_HAS_ANOMALY == 1
  Serial.printf("[INFO] Anomaly score: %f\n", result.anomaly);
#endif

  // Clean up classification result
  String label = String(result.classification[index].label);
  label.replace("\n", "");
  label.replace("\r", "");
  label.trim();

  return label;
}

/**
 * Quickly captures and releases a frame to clear the camera buffer
 */
void capture_quick() {
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) return;
  esp_camera_fb_return(fb);
}

/**
 * Captures and processes an image for classification
 * Returns true if successful, false otherwise
 */
bool capture() {
  Serial.println("Capture image...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  // Convert image to RGB888 format
  Serial.println("Converting to RGB888...");
  dl_matrix3du_t *rgb888_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  fmt2rgb888(fb->buf, fb->len, fb->format, rgb888_matrix->item);

  // Resize image to match classifier input size
  Serial.println("Resizing the frame buffer...");
  resized_matrix = dl_matrix3du_alloc(1, EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, 3);
  image_resize_linear(resized_matrix->item, rgb888_matrix->item, EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, 3, fb->width, fb->height);

  // Clean up
  dl_matrix3du_free(rgb888_matrix);
  esp_camera_fb_return(fb);

  return true;
}

/**
 * Callback function for Edge Impulse to get image data
 * Converts RGB values to float for model input
 */
int raw_feature_get_data(size_t offset, size_t out_len, float *signal_ptr) {
  size_t pixel_ix = offset * 3;
  size_t bytes_left = out_len;
  size_t out_ptr_ix = 0;

  while (bytes_left != 0) {
    uint8_t r = resized_matrix->item[pixel_ix];
    uint8_t g = resized_matrix->item[pixel_ix + 1];
    uint8_t b = resized_matrix->item[pixel_ix + 2];
    float pixel_f = (r << 16) + (g << 8) + b;
    signal_ptr[out_ptr_ix] = pixel_f;

    out_ptr_ix++;
    pixel_ix += 3;
    bytes_left--;
  }

  return 0;
}