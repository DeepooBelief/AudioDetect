idf_component_register(SRC_DIRS "."
                                "bsp"
                                "app"
                    INCLUDE_DIRS "."
                                "bsp"
                                "app"
                    REQUIRES esp_netif nvs_flash esp_wifi driver fatfs
                                )
