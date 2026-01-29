#pragma once
// BLE GATT Server implementation for Helen protocol
// Based on protocol.md specification v1.0

#include "Arduino.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_bt_defs.h"
#include "ArduinoJson.h"
#include <vector>
#include <functional>

// Protocol UUIDs from protocol.md
#define HELEN_SERVICE_UUID        0xA000
#define CHAR_DEVICE_INFO_UUID     0xA001  // Read
#define CHAR_MESSAGE_RX_UUID      0xA002  // Write
#define CHAR_MESSAGE_TX_UUID      0xA003  // Notify
#define CHAR_CONV_CONTROL_UUID    0xA004  // Write
#define CHAR_DEVICE_STATUS_UUID   0xA005  // Notify
#define CHAR_TERMINAL_RX_UUID     0xA006  // Write
#define CHAR_TERMINAL_TX_UUID     0xA007  // Notify

#define GATTS_NUM_HANDLES 20

enum BLEState {
    BLE_UNKNOWN,     // initial / uninitialized
    BLE_OFF,         // radio fully stopped
    BLE_STARTING,    // init in progress
    BLE_IDLE,        // ready but not connected (advertising)
    BLE_CONNECTED,   // connected to app
    BLE_ERROR        // failed to initialize
};

struct BLEMessage {
    String type;           // message_chunk, message_complete, conversation_start, etc.
    String conversation_id;
    String message_id;
    String role;           // user, assistant, system
    String content;
    int chunk_index;
    int total_chunks;
    uint64_t timestamp;
};

struct BLEDeviceInfo {
    String device_id;
    String device_name;
    String firmware_version;
    std::vector<String> capabilities;
    int max_message_size;
    String protocol_version;
};

class BLEManager {
private:
    BLEManager() : state(BLE_OFF), current_rssi(-1), conn_id(0xFFFF), gatts_if(ESP_GATT_IF_NONE) {
        device_info.device_id = DEVICE_ID;
        device_info.device_name = DEVICE_NAME;
        device_info.firmware_version = FIRMWARE_VERSION;
        device_info.capabilities = {"chat", "terminal"};
        device_info.max_message_size = 512;
        device_info.protocol_version = "1.0";
    }
    
    BLEState state;
    int current_rssi;
    uint16_t conn_id;
    esp_gatt_if_t gatts_if;
    uint16_t service_handle;
    
    BLEDeviceInfo device_info;
    String active_conversation_id;
    
    // Characteristic handles
    uint16_t char_device_info_handle;
    uint16_t char_message_rx_handle;
    uint16_t char_message_tx_handle;
    uint16_t char_conv_control_handle;
    uint16_t char_device_status_handle;
    uint16_t char_terminal_rx_handle;
    uint16_t char_terminal_tx_handle;
    
    // Callbacks
    std::function<void(BLEMessage)> message_rx_callback;
    std::function<void(String)> terminal_rx_callback;
    std::function<void(String, String)> conv_control_callback;  // action, conversation_id
    
    // GAP event handler
    static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
        BLEManager& mgr = BLEManager::get();
        
        switch (event) {
            case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
                Serial.println("[BLE] Advertising data set, starting advertising...");
                esp_ble_gap_start_advertising(&mgr.adv_params);
                break;
                
            case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
                if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                    Serial.println("[BLE] Advertising started");
                    mgr.state = BLE_IDLE;
                } else {
                    Serial.printf("[BLE] Advertising start failed: %x\n", param->adv_start_cmpl.status);
                    mgr.state = BLE_ERROR;
                }
                break;
                
            default:
                break;
        }
    }
    
    // GATTS event handler
    static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
        BLEManager& mgr = BLEManager::get();
        
        // Store interface on register
        if (event == ESP_GATTS_REG_EVT) {
            if (param->reg.status == ESP_GATT_OK) {
                mgr.gatts_if = gatts_if;
            } else {
                Serial.printf("[BLE] GATT register failed: %d\n", param->reg.status);
                return;
            }
        }
        
        // Ignore events for other interfaces
        if (gatts_if != ESP_GATT_IF_NONE && gatts_if != mgr.gatts_if) {
            return;
        }
        
        switch (event) {
            case ESP_GATTS_REG_EVT:
                Serial.println("[BLE] GATT server registered");
                mgr.create_service();
                break;
                
            case ESP_GATTS_CREATE_EVT:
                Serial.printf("[BLE] Service created, handle=%d\n", param->create.service_handle);
                mgr.service_handle = param->create.service_handle;
                esp_ble_gatts_start_service(mgr.service_handle);
                mgr.add_characteristics();
                break;
                
            case ESP_GATTS_START_EVT:
                Serial.println("[BLE] Service started");
                mgr.start_advertising();
                break;
                
            case ESP_GATTS_CONNECT_EVT:
                Serial.printf("[BLE] Client connected, conn_id=%d\n", param->connect.conn_id);
                mgr.conn_id = param->connect.conn_id;
                mgr.state = BLE_CONNECTED;
                esp_ble_gap_stop_advertising();
                break;
                
            case ESP_GATTS_DISCONNECT_EVT:
                Serial.println("[BLE] Client disconnected");
                mgr.conn_id = 0xFFFF;
                mgr.state = BLE_IDLE;
                esp_ble_gap_start_advertising(&mgr.adv_params);
                break;
                
            case ESP_GATTS_WRITE_EVT:
                mgr.handle_write(param);
                break;
                
            case ESP_GATTS_READ_EVT:
                mgr.handle_read(param);
                break;
                
            default:
                break;
        }
    }
    
    void create_service() {
        esp_gatt_srvc_id_t service_id;
        service_id.is_primary = true;
        service_id.id.inst_id = 0;
        service_id.id.uuid.len = ESP_UUID_LEN_16;
        service_id.id.uuid.uuid.uuid16 = HELEN_SERVICE_UUID;
        
        esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLES);
    }
    
    void add_characteristics() {
        esp_bt_uuid_t char_uuid;
        esp_gatt_perm_t perm;
        esp_gatt_char_prop_t prop;
        
        // Device Info (Read)
        char_uuid.len = ESP_UUID_LEN_16;
        char_uuid.uuid.uuid16 = CHAR_DEVICE_INFO_UUID;
        perm = ESP_GATT_PERM_READ;
        prop = ESP_GATT_CHAR_PROP_BIT_READ;
        esp_ble_gatts_add_char(service_handle, &char_uuid, perm, prop, nullptr, nullptr);
        
        // Message RX (Write)
        char_uuid.uuid.uuid16 = CHAR_MESSAGE_RX_UUID;
        perm = ESP_GATT_PERM_WRITE;
        prop = (esp_gatt_char_prop_t)(ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR);
        esp_ble_gatts_add_char(service_handle, &char_uuid, perm, prop, nullptr, nullptr);
        
        // Message TX (Notify)
        char_uuid.uuid.uuid16 = CHAR_MESSAGE_TX_UUID;
        perm = ESP_GATT_PERM_READ;
        prop = (esp_gatt_char_prop_t)(ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_READ);
        esp_ble_gatts_add_char(service_handle, &char_uuid, perm, prop, nullptr, nullptr);
        
        // Conversation Control (Write)
        char_uuid.uuid.uuid16 = CHAR_CONV_CONTROL_UUID;
        perm = ESP_GATT_PERM_WRITE;
        prop = ESP_GATT_CHAR_PROP_BIT_WRITE;
        esp_ble_gatts_add_char(service_handle, &char_uuid, perm, prop, nullptr, nullptr);
        
        // Device Status (Notify)
        char_uuid.uuid.uuid16 = CHAR_DEVICE_STATUS_UUID;
        perm = ESP_GATT_PERM_READ;
        prop = (esp_gatt_char_prop_t)(ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_READ);
        esp_ble_gatts_add_char(service_handle, &char_uuid, perm, prop, nullptr, nullptr);
        
        // Terminal RX (Write)
        char_uuid.uuid.uuid16 = CHAR_TERMINAL_RX_UUID;
        perm = ESP_GATT_PERM_WRITE;
        prop = ESP_GATT_CHAR_PROP_BIT_WRITE;
        esp_ble_gatts_add_char(service_handle, &char_uuid, perm, prop, nullptr, nullptr);
        
        // Terminal TX (Notify)
        char_uuid.uuid.uuid16 = CHAR_TERMINAL_TX_UUID;
        perm = ESP_GATT_PERM_READ;
        prop = (esp_gatt_char_prop_t)(ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_READ);
        esp_ble_gatts_add_char(service_handle, &char_uuid, perm, prop, nullptr, nullptr);
    }
    
    esp_ble_adv_params_t adv_params = {
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };
    
    void start_advertising() {
        esp_ble_adv_data_t adv_data = {};
        adv_data.set_scan_rsp = false;
        adv_data.include_name = true;
        adv_data.include_txpower = true;
        adv_data.min_interval = 0x0006;
        adv_data.max_interval = 0x0010;
        adv_data.appearance = 0x00;
        adv_data.manufacturer_len = 0;
        adv_data.p_manufacturer_data = nullptr;
        adv_data.service_data_len = 0;
        adv_data.p_service_data = nullptr;
        adv_data.service_uuid_len = 2;
        static uint8_t service_uuid[2] = {HELEN_SERVICE_UUID & 0xFF, (HELEN_SERVICE_UUID >> 8) & 0xFF};
        adv_data.p_service_uuid = service_uuid;
        adv_data.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
        
        esp_ble_gap_config_adv_data(&adv_data);
    }
    
    void handle_write(esp_ble_gatts_cb_param_t *param) {
        // Find which characteristic was written
        uint16_t handle = param->write.handle;
        String data((char*)param->write.value, param->write.len);
        
        if (debug) Serial.printf("[BLE] Write to handle %d: %s\n", handle, data.c_str());
        
        // Parse based on characteristic (simplified - in real impl, store handles in add_char callback)
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, data);
        
        if (err && debug) {
            Serial.printf("[BLE] JSON parse error: %s\n", err.c_str());
            return;
        }
        
        // Message RX
        if (message_rx_callback && doc.containsKey("type") && doc.containsKey("content")) {
            BLEMessage msg;
            msg.type = doc["type"].as<String>();
            msg.conversation_id = doc["conversation_id"].as<String>();
            msg.message_id = doc["message_id"].as<String>();
            msg.role = doc["role"].as<String>();
            msg.content = doc["content"].as<String>();
            msg.chunk_index = doc["chunk_index"] | 0;
            msg.total_chunks = doc["total_chunks"] | 1;
            msg.timestamp = doc["timestamp"] | millis();
            message_rx_callback(msg);
        }
        // Conversation Control
        else if (conv_control_callback && doc.containsKey("action")) {
            String action = doc["action"].as<String>();
            String conv_id = doc["conversation_id"].as<String>();
            conv_control_callback(action, conv_id);
        }
        // Terminal RX
        else if (terminal_rx_callback) {
            terminal_rx_callback(data);
        }
        
        // Send write response
        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, nullptr);
        }
    }
    
    void handle_read(esp_ble_gatts_cb_param_t *param) {
        // Device Info
        JsonDocument doc;
        doc["device_id"] = device_info.device_id;
        doc["device_name"] = device_info.device_name;
        doc["firmware_version"] = device_info.firmware_version;
        JsonArray caps = doc.createNestedArray("capabilities");
        for (auto& cap : device_info.capabilities) {
            caps.add(cap);
        }
        doc["max_message_size"] = device_info.max_message_size;
        doc["protocol_version"] = device_info.protocol_version;
        
        String json;
        serializeJson(doc, json);
        
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = min((int)json.length(), ESP_GATT_MAX_ATTR_LEN);
        memcpy(rsp.attr_value.value, json.c_str(), rsp.attr_value.len);
        
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
    }

public:
    static BLEManager& get() {
        static BLEManager instance;
        return instance;
    }

    inline BLEState getState() {
        return state;
    }

    inline int getRSSI() {
        return current_rssi;
    }

    inline void init() {
        if (state != BLE_OFF) return;
        
        cpu_boost(1000, 160);
        state = BLE_STARTING;
        Serial.print("(BLEd.h):");
        
        // Release BT Classic memory
        esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
        if (ret != ESP_OK) {
            Serial.printf(" BT mem release failed: %s\n", esp_err_to_name(ret));
        }
        
        // Initialize BT controller
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        ret = esp_bt_controller_init(&bt_cfg);
        if (ret != ESP_OK) {
            Serial.printf(" BT controller init failed: %s\n", esp_err_to_name(ret));
            state = BLE_ERROR;
            return;
        }
        
        // Enable BLE mode
        ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
        if (ret != ESP_OK) {
            Serial.printf(" BT controller enable failed: %s\n", esp_err_to_name(ret));
            state = BLE_ERROR;
            return;
        }
        
        // Initialize Bluedroid
        ret = esp_bluedroid_init();
        if (ret != ESP_OK) {
            Serial.printf(" Bluedroid init failed: %s\n", esp_err_to_name(ret));
            state = BLE_ERROR;
            return;
        }
        
        ret = esp_bluedroid_enable();
        if (ret != ESP_OK) {
            Serial.printf(" Bluedroid enable failed: %s\n", esp_err_to_name(ret));
            state = BLE_ERROR;
            return;
        }
        
        // Set device name
        esp_ble_gap_set_device_name(device_info.device_name.c_str());
        
        // Register callbacks
        ret = esp_ble_gap_register_callback(gap_event_handler);
        if (ret != ESP_OK) {
            Serial.printf(" GAP register failed: %s\n", esp_err_to_name(ret));
            state = BLE_ERROR;
            return;
        }
        
        ret = esp_ble_gatts_register_callback(gatts_event_handler);
        if (ret != ESP_OK) {
            Serial.printf(" GATTS register failed: %s\n", esp_err_to_name(ret));
            state = BLE_ERROR;
            return;
        }
        
        ret = esp_ble_gatts_app_register(0);
        if (ret != ESP_OK) {
            Serial.printf(" GATTS app register failed: %s\n", esp_err_to_name(ret));
            state = BLE_ERROR;
            return;
        }
        
        Serial.println(" BLE GATT server initialized.");
    }

    inline void deinit() {
        if (state == BLE_OFF) return;
        
        Serial.print("(BLEd.h): ");
        
        esp_ble_gap_stop_advertising();
        esp_ble_gatts_app_unregister(gatts_if);
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        
        state = BLE_OFF;
        conn_id = 0xFFFF;
        gatts_if = ESP_GATT_IF_NONE;
        Serial.println("BLE has been shut down.");
    }

    bool ready() { 
        return (state == BLE_IDLE || state == BLE_CONNECTED);
    }
    
    bool connected() {
        return (state == BLE_CONNECTED);
    }
    
    // Send message to app (Message TX)
    void sendMessage(String type, String content) {
        if (!connected()) return;
        
        JsonDocument doc;
        doc["type"] = type;
        doc["content"] = content;
        doc["timestamp"] = millis();
        
        String json;
        serializeJson(doc, json);
        
        // Find Message TX characteristic handle (simplified - should store during add_char)
        // For now, notify on any handle - app will filter
        esp_ble_gatts_send_indicate(gatts_if, conn_id, char_message_tx_handle, json.length(), (uint8_t*)json.c_str(), false);
    }
    
    // Send device status (Device Status)
    void sendStatus(String status, int battery_level, String message = "") {
        if (!connected()) return;
        
        JsonDocument doc;
        doc["status"] = status;
        doc["battery_level"] = battery_level;
        if (message != "") doc["message"] = message;
        doc["timestamp"] = millis();
        
        String json;
        serializeJson(doc, json);
        
        esp_ble_gatts_send_indicate(gatts_if, conn_id, char_device_status_handle, json.length(), (uint8_t*)json.c_str(), false);
    }
    
    // Send terminal output (Terminal TX)
    void sendTerminal(String output) {
        if (!connected()) return;
        
        esp_ble_gatts_send_indicate(gatts_if, conn_id, char_terminal_tx_handle, output.length(), (uint8_t*)output.c_str(), false);
    }
    
    // Register callbacks
    void onMessageRX(std::function<void(BLEMessage)> callback) {
        message_rx_callback = callback;
    }
    
    void onTerminalRX(std::function<void(String)> callback) {
        terminal_rx_callback = callback;
    }
    
    void onConversationControl(std::function<void(String, String)> callback) {
        conv_control_callback = callback;
    }
    
    String getActiveConversationId() {
        return active_conversation_id;
    }
    
    void setActiveConversationId(String conv_id) {
        active_conversation_id = conv_id;
    }
};
