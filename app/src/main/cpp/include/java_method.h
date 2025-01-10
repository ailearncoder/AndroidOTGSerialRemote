//
// Created by panxuesen on 2024/12/27.
//

#ifndef SERIALSERVER_JAVA_METHOD_H
#define SERIALSERVER_JAVA_METHOD_H

#ifdef __cplusplus
extern "C" {
#endif
// cc.axyz.serialserver.Serial.openSerial(int id)
int JavaMethod_OpenSerial(int id);
int JavaMethod_CloseSerial(int id);
int JavaMethod_ConfigureSerial(int id, int baudRate, int dataBits, float stopBits, char parity);
int JavaMethod_ReadSerial(int id, int size, int timeout, int8_t **data);
int JavaMethod_WriteSerial(int id, int8_t *data, int length, int timeout);
int JavaMethod_RtsSerialSet(int id, bool state);
bool JavaMethod_RtsSerialGet(int id);
int JavaMethod_DtrSerialSet(int id, bool state);
bool JavaMethod_DtrSerialGet(int id);
int JavaMethod_StatusSerial(int id, const char *name);
int JavaMethod_InWaitingSerial(int id);
bool JavaMethod_ResetInputBufferSerial(int id);
#ifdef __cplusplus
}
#endif

#endif //SERIALSERVER_JAVA_METHOD_H

