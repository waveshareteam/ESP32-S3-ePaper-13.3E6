idf_component_register(
  SRCS "power_bsp.c"
  PRIV_REQUIRES esp_timer driver esp_adc  
  INCLUDE_DIRS "./")
