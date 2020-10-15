/*
 * inv-helper.h
 *
 *  Created on: Dec 29, 2017
 *      Author: erhany
 */

#ifndef INV_HELPER_H_
#define INV_HELPER_H_

#define INV_NATIVE_DEVICE_NAME "invhelper"
#define INV_NATIVE_DEVICE_FILE_NAME "/dev/invhelper"

#define MAX_KEY_BLOCK_SIZE 128

#define BATTERY_STATUS_CHARGING 0
#define BATTERY_STATUS_NOT_CHARGING 1

/**
 * Stores keystore item data
 */
typedef struct keystore_item {
	unsigned short keySize; // keySize must be in AES block size: AES_BLOCK_PAD_KEY(keystoreData.keySize)
	unsigned int keySlot; // slot id in the keystore. optional when writing
	unsigned char  keyEncrypted; // set to true if you want to encrypt key in secure mem
	unsigned char 	key[MAX_KEY_BLOCK_SIZE]; // allocate a buffer before calling (size must be keysize+48 if the key is BLACK)
} key_storage;

#define MASTER_KEY_SIZE 32

/**
 * Stores the master key data
 */
typedef struct _master_key {
	unsigned char key[MASTER_KEY_SIZE];
} master_key;

#define AES_BLOCK_PAD_KEY(x) ((x % 16) ? ((x >> 4) + 1) << 4 : x)

typedef struct {
	unsigned int tx; /* Active tamper no: should be 9 */
	unsigned int rx; /* External tamper pin: should be 4 */
}tamper_active;

typedef struct {
	unsigned int rx; /* External tamper pin: should be 4 */
	unsigned int polarity;
}tamper_passive;

typedef struct {
	unsigned int lpsr; /* Check for 9. & 10. bit of lpsr and any of the first 8 bits of lptdsr is 1 to check if tamper is detected (bits count start from 0) */
	unsigned int lptdsr;
	unsigned int tamper_det_cfg; /* If 9. & 10. bit of tamper_det_cfg and all of the first 8 bits of tamper_det_cfg2 is zero then tamper is disabled (bits count start from 0) */
	unsigned int tamper_det_cfg2;
}tamper_status;

#define MAX_ALGORITHM_SIZE 16
#define MAX_IV_SIZE 32
#define MAX_INPUT_SIZE 8192
#define MAX_OUTPUT_SIZE 8192
#define MAX_KEY_SIZE 128
/**
 * Type for encryption and decryption
 *
 * For using master key ZMK:
 * 		keySize = 0
 *
 * For using key slots that are loaded into secure memory
 * 		keySlot >= 0
 * 		keySize > 0
 *
 * For using custom security keys
 * 		keySize > 0
 * 		key[] must be filled with key
 */
typedef struct {
	char algorithm[MAX_ALGORITHM_SIZE]; 	// name of algo like "cbc(aes)". Refer to kernel crypto docs
	int algorithmSize;  					// size of the char array "algorithm"
	char iv[MAX_IV_SIZE];					// iv data
	int ivSize;								// size of iv according to required algo
	unsigned char input[MAX_INPUT_SIZE];	// input buffer
	unsigned long inputSize;				// size of the char array "inputSize"
	unsigned char output[MAX_OUTPUT_SIZE];	// result buffer
	unsigned long outputSize;				// size of the char array "outputSize"
	unsigned char mode; 					// 0: encrypt CRYPTO_MODE_ENC - 1: decrypt CRYPTO_MODE_DEC
	int keySlot;							// ID of the keyslot. Uses ZMK (master key) if set to -1
	unsigned char keySize;					// Size of the key provided either by key array or keySlot. Set to 0 when using master key
	unsigned char key[MAX_KEY_SIZE];		// key: Key to be used for crypto operation. Set keySize to 0 if not used
} crypto_request;

#define CRYPTO_MODE_ENC 0
#define CRYPTO_MODE_DEC 1

#define IOCTL_INV_SUCCESS    		0 /**< Success. */
#define IOCTL_INV_INVALID_CMD		-EINVAL /**< Invalid command passed. */
#define IOCTL_INV_ZMK_HWP_SET		-100 // ZMK locked by hardware
#define IOCTL_INV_ZMK_RWL_SET		-101 // ZMK read write lock
#define IOCTL_INV_ZMK_WRITE_ERR		-102 // Written key not verified

/**
 * Used to allocate a keyslot and set a key in secure memory
 * struct keystore_data must be sent with the IOCTL command
 * input: 	keySize: Power of 16
 * 			key: Key to be set
 * output: 	keySlot: Reserved id of the slot
 */
#define INV_IOCTL_BASE					(0x10)
/**
 * Stores a key in the secure memory keystore
 */
#define INV_IOCTL_STORE_KEY 			_IOWR(INV_IOCTL_BASE, 1, struct keystore_item)
/**
 * Reads a key from key store
 */
#define INV_IOCTL_READ_KEY 				_IOWR(INV_IOCTL_BASE, 2, struct keystore_item)
/**
 * Export a key from the slot to be saved into flash
 */
#define INV_IOCTL_EXPORT_SLOT 			_IOWR(INV_IOCTL_BASE, 3, struct keystore_item)
/**
 * Import a slot from the filesystem to secure memory area
 */
#define INV_IOCTL_IMPORT_SLOT 			_IOWR(INV_IOCTL_BASE, 4, struct keystore_item)
/**
 * Enable tamper
 */
#define INV_IOCTL_TAMPER_ACTIVE_EN 		_IOWR(INV_IOCTL_BASE, 5, tamper_active)
/**
 * Disable tamper
 */
#define INV_IOCTL_TAMPER_ACTIVE_DIS 	_IOWR(INV_IOCTL_BASE, 6, tamper_active)
/**
 * N/A
 */
#define INV_IOCTL_TAMPER_PASSIVE_EN 	_IOWR(INV_IOCTL_BASE, 7, tamper_passive)
/**
 * N/A
 */
#define INV_IOCTL_TAMPER_PASSIVE_DIS 	_IOWR(INV_IOCTL_BASE, 8, tamper_passive)
/**
 * Returns tamper status register values
 */
#define INV_IOCTL_TAMPER_GET_STATUS 	_IOWR(INV_IOCTL_BASE, 9, tamper_status)
/**
 * Clears tamper status registers
 */
#define INV_IOCTL_TAMPER_CLEAR_STATUS 	_IOR(INV_IOCTL_BASE, 10, tamper_status)
/**
 * Stores the master key to secure area
 */
#define INV_IOCTL_STORE_MASTER_KEY 		_IOWR(INV_IOCTL_BASE, 11, master_key)
/**
 * Retrieves the master key to the memory
 */
#define INV_IOCTL_READ_MASTER_KEY 		_IOWR(INV_IOCTL_BASE, 12, master_key)
/**
 * Enable or disable finger print sensor power
 */
#define INV_IOCTL_SET_FINGER_PRINT_SENSOR_ENABLE 	_IOWR(INV_IOCTL_BASE, 13, unsigned char)
/**
 * Enable or disable wifi module power
 */
#define INV_IOCTL_SET_WIFI_MODULE_ENABLE 			_IOWR(INV_IOCTL_BASE, 14, unsigned char)
/**
 * Enable or disable battery charging
 */
#define INV_IOCTL_SET_BATTERY_CHARGE_ON 			_IOWR(INV_IOCTL_BASE, 15, unsigned char)
/**
 * Get charging status as integer.
 * Returns BATTERY_STATUS_CHARGING or BATTERY_STATUS_NOT_CHARGING
 */
#define INV_IOCTL_GET_BATTERY_CHARGE_STATUS			_IOWR(INV_IOCTL_BASE, 16, unsigned int)

/**
 * Cryptographic operation request with ZMK (master key)
 */
#define INV_IOCTL_CRYPTO_REQUEST					_IOWR(INV_IOCTL_BASE, 17, unsigned int)

/**
 * Enable or disable finger print sensor power
 */
#define INV_IOCTL_SET_GSM_MODEM_ENABLE 	_IOWR(INV_IOCTL_BASE, 18, unsigned char)

/**
 * Enable or disable finger print sensor power
 */
#define INV_IOCTL_SET_ETHERNET_ENABLE 	_IOWR(INV_IOCTL_BASE, 19, unsigned char)

/**
 * First 4 status are as below:
#define SIM_STATUS_USER_CARD_INSERTED 1
#define SIM_STATUS_USER_CARD_REMOVED 2
#define SIM_STATUS_CUSTOMER_CARD_INSERTED 3
#define SIM_STATUS_CUSTOMER_CARD_REMOVED 4
 */
#define TAMPER_OCCURED 5

#endif /* INV_HELPER_H_ */
