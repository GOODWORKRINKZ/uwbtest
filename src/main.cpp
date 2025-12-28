/*! ----------------------------------------------------------------------------
 * @file    main.cpp
 * @brief   SS-TWR implementation using official Decawave driver for ESP32-C3
 * 
 * Based on Decawave ex_06a (TAG/Initiator) and ex_06b (ANCHOR/Responder)
 * Adapted for Arduino ESP32-C3 framework
 */

#include <Arduino.h>
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port_arduino.h"

// Mode selection
#define MODE_TAG    1
#define MODE_ANCHOR 2

// DEVICE_MODE will be defined by platformio.ini build_flags
// Use -e tag or -e anchor to select mode
#ifndef DEVICE_MODE
#define DEVICE_MODE MODE_TAG  // Default to TAG if not specified
#endif

#ifndef DEVICE_NAME
#if DEVICE_MODE == MODE_TAG
#define DEVICE_NAME "TAG"
#else
#define DEVICE_NAME "ANCHOR"
#endif
#endif

/* Application name */
#define APP_NAME "SS TWR " DEVICE_NAME

/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 100

/* Default communication configuration. We use here EVK1000's mode 4. */
static dwt_config_t config = {
    2,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    0,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (129 + 8 - 8)    /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

/* Default antenna delay values for 64 MHz PRF. */
#define TX_ANT_DLY 16436
#define RX_ANT_DLY 16436

/* Frames used in the ranging process. */
static uint8 tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8 rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8 rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8 tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Length of the common part of the message */
#define ALL_MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames */
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14
#define RESP_MSG_TS_LEN 4

/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Buffer to store received messages. */
#define RX_BUF_LEN 20
static uint8 rx_buffer[RX_BUF_LEN];
/* Statistics counters */
static uint32 last_stats_time = 0;
static uint32 success_count = 0;
static uint32 timeout_count = 0;
static uint32 error_count = 0;
/* Hold copy of status register state here for reference. */
static uint32 status_reg = 0;

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) conversion factor. */
#define UUS_TO_DWT_TIME 65536

/* Delay between frames, in UWB microseconds. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 140
#define POLL_RX_TO_RESP_TX_DLY_UUS 2500  /* Increased for Serial.print overhead */

/* Receive response timeout. */
#define RESP_RX_TIMEOUT_UUS 5000  /* Increased to match longer ANCHOR delay */

/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299702547

/* Hold copies of computed time of flight and distance. */
static double tof;
static double distance;

/* Timestamps of frames transmission/reception. */
typedef unsigned long long uint64;
static uint64 poll_rx_ts;
static uint64 resp_tx_ts;

/* Function prototypes */
static void resp_msg_get_ts(uint8 *ts_field, uint32 *ts);
static void resp_msg_set_ts(uint8 *ts_field, const uint64 ts);
static uint64 get_rx_timestamp_u64(void);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n================================");
    Serial.println(APP_NAME);
    Serial.println("Official Decawave Driver");
    Serial.println("================================\n");
    
    /* Initialize peripherals */
    peripherals_init();
    
    Serial.println("Testing SPI communication...");
    
    /* DW1000 power-up delay (hardware RST not connected) */
    reset_DW1000();  // Just delay for power-up
    
    /* Apply DW1000 SPI bug fix at high speed before slowrate */
    dwt_write32bitreg(SYS_CFG_ID, 0x1600);
    Serial.println("✓ Applied DW1000 SPI bug fix");
    
    /* Set SPI to slow speed for initialization */
    port_set_dw1000_slowrate();
    Serial.println("✓ SPI set to slow rate (3MHz)");
    
    /* Test Device ID */
    uint32 testID = dwt_readdevid();
    Serial.print("Device ID: 0x");
    Serial.println(testID, HEX);
    
    if (testID != 0xDECA0130) {
        Serial.println("❌ SPI COMMUNICATION FAILED!");
        Serial.print("   Expected: 0xDECA0130, Got: 0x");
        Serial.println(testID, HEX);
        while (1) { delay(1000); }
    }
    
    Serial.println("✓ Device ID OK, starting initialization...");
    
    /* Initialize DW1000 with microcode loading */
    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR) {
        Serial.println("❌ DW1000 INIT FAILED!");
        while (1) { delay(100); }
    }
    
    /* Switch to fast SPI speed after initialization */
    port_set_dw1000_fastrate();
    Serial.println("✓ DW1000 initialization complete");
    
    /* Read Device ID */
    uint32 devID = dwt_readdevid();
    Serial.print("Device ID: 0x");
    Serial.println(devID, HEX);
    
    /* Configure DW1000. */
    dwt_configure(&config);
    Serial.println("✓ DW1000 configured");
    
    /* Enable smart TX power control */
    dwt_setsmarttxpower(1);
    Serial.println("✓ Smart TX power enabled");
    
    // Verify configuration
    uint32_t chan_ctrl = dwt_read32bitreg(CHAN_CTRL_ID);
    uint32_t tx_power = dwt_read32bitreg(TX_POWER_ID);
    Serial.print("  CHAN_CTRL: 0x");
    Serial.print(chan_ctrl, HEX);
    Serial.print(", TX_POWER: 0x");
    Serial.println(tx_power, HEX);
    
    /* Apply default antenna delay value. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);
    Serial.println("✓ Antenna delays set");
    
#if DEVICE_MODE == MODE_TAG
    /* Set expected response's delay and timeout for TAG mode */
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    Serial.println("✓ TAG mode ready\n");
#else
    Serial.println("✓ ANCHOR mode ready\n");
#endif
}

void loop() {
#if DEVICE_MODE == MODE_TAG
    /* ==================== TAG MODE (Initiator) ==================== */
    
    /* Clear ALL TX status bits at start of each iteration */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS | SYS_STATUS_TXPHS | SYS_STATUS_TXPRS | SYS_STATUS_TXFRB);
    
    static uint32_t loop_count = 0;
    if (++loop_count % 100 == 0) {
        Serial.print("🔄 [TAG] Loop iteration: ");
        Serial.println(loop_count);
    }
    
    /* Write frame data to DW1000 and prepare transmission. */
    tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
    dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0);
    dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1);
    
    /* Start transmission, indicating that a response is expected. */
    int ret = dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
    if (ret != DWT_SUCCESS) {
        Serial.print("[TAG] TX failed! ret=");
        Serial.println(ret);
        delay(RNG_DELAY_MS);
        return;
    }
    
    /* Wait for TX to complete */
    uint32_t tx_wait_start = millis();
    while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS)) {
        if (millis() - tx_wait_start > 10) {
            Serial.println("[TAG] TX timeout!");
            delay(RNG_DELAY_MS);
            return;
        }
    }
    
    if (loop_count % 10 == 0) {
        Serial.println("[TAG] ✓ POLL transmitted");
    }
    
    /* Increment frame sequence number after transmission (modulo 256). */
    frame_seq_nb++;
    
    /* Poll for reception of a frame or error/timeout. */
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
    { };
    
    if (status_reg & SYS_STATUS_RXFCG) {
        uint32 frame_len;
        
        /* Clear good RX frame event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
        
        /* A frame has been received, read it into the local buffer. */
        frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
        if (frame_len <= RX_BUF_LEN) {
            dwt_readrxdata(rx_buffer, frame_len, 0);
        }

        
        /* Check that the frame is the expected response. */
        rx_buffer[ALL_MSG_SN_IDX] = 0;
        if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0) {
            uint32 poll_tx_ts, resp_rx_ts, poll_rx_ts, resp_tx_ts;
            int32 rtd_init, rtd_resp;
            float clockOffsetRatio;
            
            /* Retrieve poll transmission and response reception timestamps. */
            poll_tx_ts = dwt_readtxtimestamplo32();
            resp_rx_ts = dwt_readrxtimestamplo32();
            
            /* Read carrier integrator value and calculate clock offset ratio. */
            clockOffsetRatio = dwt_readcarrierintegrator() * (FREQ_OFFSET_MULTIPLIER * HERTZ_TO_PPM_MULTIPLIER_CHAN_2 / 1.0e6);
            
            /* Get timestamps embedded in response message. */
            resp_msg_get_ts(&rx_buffer[RESP_MSG_POLL_RX_TS_IDX], &poll_rx_ts);
            resp_msg_get_ts(&rx_buffer[RESP_MSG_RESP_TX_TS_IDX], &resp_tx_ts);
            
            /* Compute time of flight and distance. */
            rtd_init = resp_rx_ts - poll_tx_ts;
            rtd_resp = resp_tx_ts - poll_rx_ts;
            
            tof = ((rtd_init - rtd_resp * (1 - clockOffsetRatio)) / 2.0) * DWT_TIME_UNITS;
            distance = tof * SPEED_OF_LIGHT;
            
            /* Display computed distance in Teleplot format with timestamp and unit. */
            success_count++;
            Serial.print(">distance:");
            Serial.print(millis());
            Serial.print(":");
            Serial.print(distance, 2);
            Serial.println("§m");
        }
        
        /* Force DW1000 back to IDLE after successful RX */
        dwt_forcetrxoff();
    } else {
        /* Clear RX error/timeout events in the DW1000 status register. */
        if (status_reg & SYS_STATUS_ALL_RX_TO) {
            timeout_count++;
            if (timeout_count % 10 == 0) {
                Serial.print("[TAG] RX Timeout! Status: 0x");
                Serial.println(status_reg, HEX);
            }
        } else {
            error_count++;
            Serial.print("[TAG] RX Error! Status: 0x");
            Serial.print(status_reg, HEX);
            if (status_reg & SYS_STATUS_RXPHE) Serial.print(" RXPHE");
            if (status_reg & SYS_STATUS_RXRFSL) Serial.print(" RXRFSL");
            if (status_reg & SYS_STATUS_LDEERR) Serial.print(" LDEERR");
            Serial.println();
        }
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        
        /* Reset RX to properly reinitialise LDE operation. */
        dwt_rxreset();
    }
    
    /* Print statistics every 1000ms */
    uint32 now = millis();
    if (now - last_stats_time >= 1000) {
        Serial.print("[TAG] Stats - Success: ");
        Serial.print(success_count);
        Serial.print(", Timeout: ");
        Serial.print(timeout_count);
        Serial.print(", Error: ");
        Serial.println(error_count);
        last_stats_time = now;
    }
    
    /* Execute a delay between ranging exchanges. */
    delay(RNG_DELAY_MS);
    
#else
    /* ==================== ANCHOR MODE (Responder) ==================== */
    
    /* Clear all status bits before starting RX */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_GOOD | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
    
    /* Activate reception immediately */
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    
    /* Poll for reception of a frame or error/timeout */
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR)))
    { };
    
    if (status_reg & SYS_STATUS_RXFCG) {
        uint32 frame_len;
        
        /* Clear good RX frame event in the DW1000 status register in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
        
        /* A frame has been received, read it into the local buffer. */
        frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
        if (frame_len <= RX_BUF_LEN) {
            dwt_readrxdata(rx_buffer, frame_len, 0);
        }
        
        /* Check that the frame is a poll sent by "SS TWR initiator" example.
         * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
        uint8 saved_seq = rx_buffer[ALL_MSG_SN_IDX];
        rx_buffer[ALL_MSG_SN_IDX] = 0;
        
        if (memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0) {
            uint32 resp_tx_time;
            int ret;
            
            /* Retrieve poll reception timestamp. */
            poll_rx_ts = get_rx_timestamp_u64();
            
            /* Compute final message transmission time. */
            resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
            dwt_setdelayedtrxtime(resp_tx_time);
            
            /* Response TX timestamp is the transmission time we programmed plus the antenna delay. */
            resp_tx_ts = (((uint64)(resp_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;
            
            /* Write all timestamps in the response message. */
            resp_msg_set_ts(&tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX], poll_rx_ts);
            resp_msg_set_ts(&tx_resp_msg[RESP_MSG_RESP_TX_TS_IDX], resp_tx_ts);
            
            /* Write and send the response message. */
            tx_resp_msg[ALL_MSG_SN_IDX] = saved_seq;
            dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0);
            dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1);
            ret = dwt_starttx(DWT_START_TX_DELAYED);
            
            /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. */
            if (ret == DWT_SUCCESS) {
                /* Poll DW1000 until TX frame sent event set. */
                while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
                { };
                
                /* Clear TXFRS event. */
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
                
                /* Force DW1000 back to IDLE after successful TX */
                dwt_forcetrxoff();
                
                /* Increment frame sequence number after transmission of the response message (modulo 256). */
                frame_seq_nb++;
                success_count++;
            } else {
                error_count++;
            }
        }
    } else {
        /* Clear RX error events in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        
        /* Reset RX to properly reinitialise LDE operation. */
        dwt_rxreset();
        error_count++;
    }
    
    /* Print statistics every 1000ms */
    uint32 now = millis();
    if (now - last_stats_time >= 1000) {
        Serial.print("[ANCHOR] Stats - RX: ");
        Serial.print(success_count);
        Serial.print(", Err: ");
        Serial.println(error_count);
        last_stats_time = now;
    }
#endif
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn resp_msg_get_ts()
 *
 * @brief Read a given timestamp value from the response message.
 */
static void resp_msg_get_ts(uint8 *ts_field, uint32 *ts)
{
    int i;
    *ts = 0;
    for (i = 0; i < RESP_MSG_TS_LEN; i++)
    {
        *ts += ts_field[i] << (i * 8);
    }
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn get_rx_timestamp_u64()
 *
 * @brief Get the RX time-stamp in a 64-bit variable.
 */
static uint64 get_rx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;
    int i;
    dwt_readrxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--)
    {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn resp_msg_set_ts()
 *
 * @brief Fill a given timestamp field in the response message with the given value.
 */
static void resp_msg_set_ts(uint8 *ts_field, const uint64 ts)
{
    int i;
    for (i = 0; i < RESP_MSG_TS_LEN; i++)
    {
        ts_field[i] = (ts >> (i * 8)) & 0xFF;
    }
}
