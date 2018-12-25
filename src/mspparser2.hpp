/*
   mspparser.hpp: header-only implementation of MSP parsing routines

   Auto-generated code: DO NOT EDIT!

   Copyright (C) Simon D. Levy 2018

   This program is part of Hackflight

   This code is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as 
   published by the Free Software Foundation, either version 3 of the 
   License, or (at your option) any later version.

   This code is distributed in the hope that it will be useful,     
   but WITHOUT ANY WARRANTY without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License 
   along with this code.  If not, see <http:#www.gnu.org/licenses/>.
 */


#pragma once

#include <stdint.h>
#include <string.h>

namespace hf {

    class MspParser {

        public:

            static const uint8_t MAXMSG = 255;

        private:

            static const int INBUF_SIZE  = 128;
            static const int OUTBUF_SIZE = 128;

            typedef enum serialState_t {
                IDLE,
                HEADER_START,
                HEADER_M,
                HEADER_ARROW,
                HEADER_SIZE,
                HEADER_CMD
            } serialState_t;

            uint8_t _checksum;
            uint8_t _inBuf[INBUF_SIZE];
            uint8_t _inBufIndex;
            uint8_t _outBuf[OUTBUF_SIZE];
            uint8_t _outBufIndex;
            uint8_t _outBufSize;
            uint8_t _command;
            uint8_t _offset;
            uint8_t _dataSize;
            uint8_t _direction;

            serialState_t  _state;

            void serialize8(uint8_t a)
            {
                _outBuf[_outBufSize++] = a;
                _checksum ^= a;
            }

            void serialize16(int16_t a)
            {
                serialize8(a & 0xFF);
                serialize8((a >> 8) & 0xFF);
            }

            void serialize32(uint32_t a)
            {
                serialize8(a & 0xFF);
                serialize8((a >> 8) & 0xFF);
                serialize8((a >> 16) & 0xFF);
                serialize8((a >> 24) & 0xFF);
            }

            void headSerialResponse(uint8_t err, uint8_t s)
            {
                serialize8('$');
                serialize8('M');
                serialize8(err ? '!' : '>');
                _checksum = 0;               // start calculating a new _checksum
                serialize8(s);
                serialize8(_command);
            }

            void headSerialReply(uint8_t s)
            {
                headSerialResponse(0, s);
            }

            void prepareToSend(uint8_t count, uint8_t size)
            {
                _outBufSize = 0;
                _outBufIndex = 0;
                headSerialReply(count*size);
            }

            void prepareToSendBytes(uint8_t count)
            {
                prepareToSend(count, 1);
            }

            void sendByte(uint8_t src)
            {
                serialize8(src);
            }

            void prepareToSendShorts(uint8_t count)
            {
                prepareToSend(count, 2);
            }

            void sendShort(short src)
            {
                int16_t a;
                memcpy(&a, &src, 2);
                serialize16(a);
            }

            void prepareToSendInts(uint8_t count)
            {
                prepareToSend(count, 4);
            }

            void sendInt(int32_t src)
            {
                int32_t a;
                memcpy(&a, &src, 4);
                serialize32(a);
            }

            void prepareToSendFloats(uint8_t count)
            {
                prepareToSend(count, 4);
            }

            void sendFloat(float src)
            {
                uint32_t a;
                memcpy(&a, &src, 4);
                serialize32(a);
            }

            static uint8_t CRC8(uint8_t * data, int n) 
            {
                uint8_t crc = 0x00;

                for (int k=0; k<n; ++k) {

                    crc ^= data[k];
                }

                return crc;
            }

            float getArgument(uint8_t k)
            {
                return (float)k; // XXX for testing only
            }

        protected:

            void init(void)
            {
                _checksum = 0;
                _outBufIndex = 0;
                _outBufSize = 0;
                _command = 0;
                _offset = 0;
                _dataSize = 0;
                _state = IDLE;
            }
            
            uint8_t availableBytes(void)
            {
                return _outBufSize;
            }

            uint8_t readByte(void)
            {
                _outBufSize--;
                return _outBuf[_outBufIndex++];
            }

            // returns true if reboot request, false otherwise
            bool parse(uint8_t c)
            {
                switch (_state) {

                    case IDLE:
                        if (c == 'R') {
                            return true; // got reboot command
                        }
                        _state = (c == '$') ? HEADER_START : IDLE;
                        break;

                    case HEADER_START:
                        _state = (c == 'M') ? HEADER_M : IDLE;
                        break;

                    case HEADER_M:
                        switch (c) {
                           case '>':
                                _direction = 1;
                                _state = HEADER_ARROW;
                                break;
                            case '<':
                                _direction = 0;
                                _state = HEADER_ARROW;
                                break;
                             default:
                                _state = IDLE;
                        }
                        break;

                    case HEADER_ARROW:
                        if (c > INBUF_SIZE) {       // now we are expecting the payload size
                            _state = IDLE;
                            return false;
                        }
                        _dataSize = c;
                        _offset = 0;
                        _checksum = 0;
                        _inBufIndex = 0;
                        _checksum ^= c;
                        _state = HEADER_SIZE;      // the command is to follow
                        break;

                    case HEADER_SIZE:
                        _command = c;
                        _checksum ^= c;
                        _state = HEADER_CMD;
                        break;

                    case HEADER_CMD:
                        if (_offset < _dataSize) {
                            _checksum ^= c;
                            _inBuf[_offset++] = c;
                        } else  {
                            if (_checksum == c) {        // compare calculated and transferred _checksum
                                if (_direction == 0) {
                                    dispatchRequestMessage();
                                }
                                else {
                                    dispatchDataMessage();
                                }
                            }
                            _state = IDLE;
                        }

                } // switch (_state)

                return false; // no reboot 

            } // parse


            void dispatchRequestMessage(void)
            {
                switch (_command) {

                    case 102:
                    {
                        int16_t accx = 0;
                        int16_t accy = 0;
                        int16_t accz = 0;
                        int16_t gyrx = 0;
                        int16_t gyry = 0;
                        int16_t gyrz = 0;
                        int16_t magx = 0;
                        int16_t magy = 0;
                        int16_t magz = 0;
                        handle_RAW_IMU_Request(accx, accy, accz, gyrx, gyry, gyrz, magx, magy, magz);
                        prepareToSendShorts(9);
                        sendShort(accx);
                        sendShort(accy);
                        sendShort(accz);
                        sendShort(gyrx);
                        sendShort(gyry);
                        sendShort(gyrz);
                        sendShort(magx);
                        sendShort(magy);
                        sendShort(magz);
                        serialize8(_checksum);
                        } break;

                    case 112:
                    {
                        float altitude = 0;
                        float variometer = 0;
                        float positionX = 0;
                        float positionY = 0;
                        float heading = 0;
                        float velocityForward = 0;
                        float velocityRightward = 0;
                        handle_STATE_Request(altitude, variometer, positionX, positionY, heading, velocityForward, velocityRightward);
                        prepareToSendFloats(7);
                        sendFloat(altitude);
                        sendFloat(variometer);
                        sendFloat(positionX);
                        sendFloat(positionY);
                        sendFloat(heading);
                        sendFloat(velocityForward);
                        sendFloat(velocityRightward);
                        serialize8(_checksum);
                        } break;

                    case 121:
                    {
                        float c1 = 0;
                        float c2 = 0;
                        float c3 = 0;
                        float c4 = 0;
                        float c5 = 0;
                        float c6 = 0;
                        handle_RC_NORMAL_Request(c1, c2, c3, c4, c5, c6);
                        prepareToSendFloats(6);
                        sendFloat(c1);
                        sendFloat(c2);
                        sendFloat(c3);
                        sendFloat(c4);
                        sendFloat(c5);
                        sendFloat(c6);
                        serialize8(_checksum);
                        } break;

                    case 122:
                    {
                        float roll = 0;
                        float pitch = 0;
                        float yaw = 0;
                        handle_ATTITUDE_RADIANS_Request(roll, pitch, yaw);
                        prepareToSendFloats(3);
                        sendFloat(roll);
                        sendFloat(pitch);
                        sendFloat(yaw);
                        serialize8(_checksum);
                        } break;

                    case 123:
                    {
                        float estalt = 0;
                        float vario = 0;
                        handle_ALTITUDE_METERS_Request(estalt, vario);
                        prepareToSendFloats(2);
                        sendFloat(estalt);
                        sendFloat(vario);
                        serialize8(_checksum);
                        } break;

                    case 126:
                    {
                        int16_t range = 0;
                        int16_t flowx = 0;
                        int16_t flowy = 0;
                        handle_RANGE_AND_FLOW_Request(range, flowx, flowy);
                        prepareToSendShorts(3);
                        sendShort(range);
                        sendShort(flowx);
                        sendShort(flowy);
                        serialize8(_checksum);
                        } break;

                    case 215:
                    {
                        float m1 = 0;
                        memcpy(&m1,  &_inBuf[0], sizeof(float));

                        float m2 = 0;
                        memcpy(&m2,  &_inBuf[4], sizeof(float));

                        float m3 = 0;
                        memcpy(&m3,  &_inBuf[8], sizeof(float));

                        float m4 = 0;
                        memcpy(&m4,  &_inBuf[12], sizeof(float));

                        handle_SET_MOTOR_NORMAL_Request(m1, m2, m3, m4);
                        } break;

                    case 216:
                    {
                        uint8_t flag = 0;
                        memcpy(&flag,  &_inBuf[0], sizeof(uint8_t));

                        handle_SET_ARMED_Request(flag);
                        } break;

                    case 222:
                    {
                        float c1 = 0;
                        memcpy(&c1,  &_inBuf[0], sizeof(float));

                        float c2 = 0;
                        memcpy(&c2,  &_inBuf[4], sizeof(float));

                        float c3 = 0;
                        memcpy(&c3,  &_inBuf[8], sizeof(float));

                        float c4 = 0;
                        memcpy(&c4,  &_inBuf[12], sizeof(float));

                        float c5 = 0;
                        memcpy(&c5,  &_inBuf[16], sizeof(float));

                        float c6 = 0;
                        memcpy(&c6,  &_inBuf[20], sizeof(float));

                        handle_SET_RC_NORMAL_Request(c1, c2, c3, c4, c5, c6);
                        } break;

                    case 226:
                    {
                        int16_t range = 0;
                        memcpy(&range,  &_inBuf[0], sizeof(int16_t));

                        int16_t flowx = 0;
                        memcpy(&flowx,  &_inBuf[2], sizeof(int16_t));

                        int16_t flowy = 0;
                        memcpy(&flowy,  &_inBuf[4], sizeof(int16_t));

                        handle_SET_RANGE_AND_FLOW_Request(range, flowx, flowy);
                        } break;

                }
            }

            void dispatchDataMessage(void)
            {
                switch (_command) {

                    case 102:
                    {
                        int16_t accx = getArgument(0);
                        int16_t accy = getArgument(1);
                        int16_t accz = getArgument(2);
                        int16_t gyrx = getArgument(3);
                        int16_t gyry = getArgument(4);
                        int16_t gyrz = getArgument(5);
                        int16_t magx = getArgument(6);
                        int16_t magy = getArgument(7);
                        int16_t magz = getArgument(8);
                        handle_RAW_IMU_Data(accx, accy, accz, gyrx, gyry, gyrz, magx, magy, magz);
                        } break;

                    case 112:
                    {
                        float altitude = getArgument(0);
                        float variometer = getArgument(1);
                        float positionX = getArgument(2);
                        float positionY = getArgument(3);
                        float heading = getArgument(4);
                        float velocityForward = getArgument(5);
                        float velocityRightward = getArgument(6);
                        handle_STATE_Data(altitude, variometer, positionX, positionY, heading, velocityForward, velocityRightward);
                        } break;

                    case 121:
                    {
                        float c1 = getArgument(0);
                        float c2 = getArgument(1);
                        float c3 = getArgument(2);
                        float c4 = getArgument(3);
                        float c5 = getArgument(4);
                        float c6 = getArgument(5);
                        handle_RC_NORMAL_Data(c1, c2, c3, c4, c5, c6);
                        } break;

                    case 122:
                    {
                        float roll = getArgument(0);
                        float pitch = getArgument(1);
                        float yaw = getArgument(2);
                        handle_ATTITUDE_RADIANS_Data(roll, pitch, yaw);
                        } break;

                    case 123:
                    {
                        float estalt = getArgument(0);
                        float vario = getArgument(1);
                        handle_ALTITUDE_METERS_Data(estalt, vario);
                        } break;

                    case 126:
                    {
                        int16_t range = getArgument(0);
                        int16_t flowx = getArgument(1);
                        int16_t flowy = getArgument(2);
                        handle_RANGE_AND_FLOW_Data(range, flowx, flowy);
                        } break;

                }
            }

        protected:

            virtual void handle_RAW_IMU_Request(int16_t & accx, int16_t & accy, int16_t & accz, int16_t & gyrx, int16_t & gyry, int16_t & gyrz, int16_t & magx, int16_t & magy, int16_t & magz)
            {
                (void)accx;
                (void)accy;
                (void)accz;
                (void)gyrx;
                (void)gyry;
                (void)gyrz;
                (void)magx;
                (void)magy;
                (void)magz;
            }

            virtual void handle_RAW_IMU_Data(int16_t & accx, int16_t & accy, int16_t & accz, int16_t & gyrx, int16_t & gyry, int16_t & gyrz, int16_t & magx, int16_t & magy, int16_t & magz)
            {
                (void)accx;
                (void)accy;
                (void)accz;
                (void)gyrx;
                (void)gyry;
                (void)gyrz;
                (void)magx;
                (void)magy;
                (void)magz;
            }

            virtual void handle_STATE_Request(float & altitude, float & variometer, float & positionX, float & positionY, float & heading, float & velocityForward, float & velocityRightward)
            {
                (void)altitude;
                (void)variometer;
                (void)positionX;
                (void)positionY;
                (void)heading;
                (void)velocityForward;
                (void)velocityRightward;
            }

            virtual void handle_STATE_Data(float & altitude, float & variometer, float & positionX, float & positionY, float & heading, float & velocityForward, float & velocityRightward)
            {
                (void)altitude;
                (void)variometer;
                (void)positionX;
                (void)positionY;
                (void)heading;
                (void)velocityForward;
                (void)velocityRightward;
            }

            virtual void handle_RC_NORMAL_Request(float & c1, float & c2, float & c3, float & c4, float & c5, float & c6)
            {
                (void)c1;
                (void)c2;
                (void)c3;
                (void)c4;
                (void)c5;
                (void)c6;
            }

            virtual void handle_RC_NORMAL_Data(float & c1, float & c2, float & c3, float & c4, float & c5, float & c6)
            {
                (void)c1;
                (void)c2;
                (void)c3;
                (void)c4;
                (void)c5;
                (void)c6;
            }

            virtual void handle_ATTITUDE_RADIANS_Request(float & roll, float & pitch, float & yaw)
            {
                (void)roll;
                (void)pitch;
                (void)yaw;
            }

            virtual void handle_ATTITUDE_RADIANS_Data(float & roll, float & pitch, float & yaw)
            {
                (void)roll;
                (void)pitch;
                (void)yaw;
            }

            virtual void handle_ALTITUDE_METERS_Request(float & estalt, float & vario)
            {
                (void)estalt;
                (void)vario;
            }

            virtual void handle_ALTITUDE_METERS_Data(float & estalt, float & vario)
            {
                (void)estalt;
                (void)vario;
            }

            virtual void handle_RANGE_AND_FLOW_Request(int16_t & range, int16_t & flowx, int16_t & flowy)
            {
                (void)range;
                (void)flowx;
                (void)flowy;
            }

            virtual void handle_RANGE_AND_FLOW_Data(int16_t & range, int16_t & flowx, int16_t & flowy)
            {
                (void)range;
                (void)flowx;
                (void)flowy;
            }

            virtual void handle_SET_MOTOR_NORMAL_Request(float  m1, float  m2, float  m3, float  m4)
            {
                (void)m1;
                (void)m2;
                (void)m3;
                (void)m4;
            }

            virtual void handle_SET_MOTOR_NORMAL_Data(float  m1, float  m2, float  m3, float  m4)
            {
                (void)m1;
                (void)m2;
                (void)m3;
                (void)m4;
            }

            virtual void handle_SET_ARMED_Request(uint8_t  flag)
            {
                (void)flag;
            }

            virtual void handle_SET_ARMED_Data(uint8_t  flag)
            {
                (void)flag;
            }

            virtual void handle_SET_RC_NORMAL_Request(float  c1, float  c2, float  c3, float  c4, float  c5, float  c6)
            {
                (void)c1;
                (void)c2;
                (void)c3;
                (void)c4;
                (void)c5;
                (void)c6;
            }

            virtual void handle_SET_RC_NORMAL_Data(float  c1, float  c2, float  c3, float  c4, float  c5, float  c6)
            {
                (void)c1;
                (void)c2;
                (void)c3;
                (void)c4;
                (void)c5;
                (void)c6;
            }

            virtual void handle_SET_RANGE_AND_FLOW_Request(int16_t  range, int16_t  flowx, int16_t  flowy)
            {
                (void)range;
                (void)flowx;
                (void)flowy;
            }

            virtual void handle_SET_RANGE_AND_FLOW_Data(int16_t  range, int16_t  flowx, int16_t  flowy)
            {
                (void)range;
                (void)flowx;
                (void)flowy;
            }

        public:

            static uint8_t serialize_RAW_IMU_Request(uint8_t bytes[])
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 60;
                bytes[3] = 0;
                bytes[4] = 102;
                bytes[5] = 102;

                return 6;
            }

            static uint8_t serialize_RAW_IMU(uint8_t bytes[], int16_t  accx, int16_t  accy, int16_t  accz, int16_t  gyrx, int16_t  gyry, int16_t  gyrz, int16_t  magx, int16_t  magy, int16_t  magz)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 18;
                bytes[4] = 102;

                memcpy(&bytes[5], &accx, sizeof(int16_t));
                memcpy(&bytes[7], &accy, sizeof(int16_t));
                memcpy(&bytes[9], &accz, sizeof(int16_t));
                memcpy(&bytes[11], &gyrx, sizeof(int16_t));
                memcpy(&bytes[13], &gyry, sizeof(int16_t));
                memcpy(&bytes[15], &gyrz, sizeof(int16_t));
                memcpy(&bytes[17], &magx, sizeof(int16_t));
                memcpy(&bytes[19], &magy, sizeof(int16_t));
                memcpy(&bytes[21], &magz, sizeof(int16_t));

                bytes[23] = CRC8(&bytes[3], 20);

                return 24;
            }

            static uint8_t serialize_STATE_Request(uint8_t bytes[])
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 60;
                bytes[3] = 0;
                bytes[4] = 112;
                bytes[5] = 112;

                return 6;
            }

            static uint8_t serialize_STATE(uint8_t bytes[], float  altitude, float  variometer, float  positionX, float  positionY, float  heading, float  velocityForward, float  velocityRightward)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 28;
                bytes[4] = 112;

                memcpy(&bytes[5], &altitude, sizeof(float));
                memcpy(&bytes[9], &variometer, sizeof(float));
                memcpy(&bytes[13], &positionX, sizeof(float));
                memcpy(&bytes[17], &positionY, sizeof(float));
                memcpy(&bytes[21], &heading, sizeof(float));
                memcpy(&bytes[25], &velocityForward, sizeof(float));
                memcpy(&bytes[29], &velocityRightward, sizeof(float));

                bytes[33] = CRC8(&bytes[3], 30);

                return 34;
            }

            static uint8_t serialize_RC_NORMAL_Request(uint8_t bytes[])
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 60;
                bytes[3] = 0;
                bytes[4] = 121;
                bytes[5] = 121;

                return 6;
            }

            static uint8_t serialize_RC_NORMAL(uint8_t bytes[], float  c1, float  c2, float  c3, float  c4, float  c5, float  c6)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 24;
                bytes[4] = 121;

                memcpy(&bytes[5], &c1, sizeof(float));
                memcpy(&bytes[9], &c2, sizeof(float));
                memcpy(&bytes[13], &c3, sizeof(float));
                memcpy(&bytes[17], &c4, sizeof(float));
                memcpy(&bytes[21], &c5, sizeof(float));
                memcpy(&bytes[25], &c6, sizeof(float));

                bytes[29] = CRC8(&bytes[3], 26);

                return 30;
            }

            static uint8_t serialize_ATTITUDE_RADIANS_Request(uint8_t bytes[])
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 60;
                bytes[3] = 0;
                bytes[4] = 122;
                bytes[5] = 122;

                return 6;
            }

            static uint8_t serialize_ATTITUDE_RADIANS(uint8_t bytes[], float  roll, float  pitch, float  yaw)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 12;
                bytes[4] = 122;

                memcpy(&bytes[5], &roll, sizeof(float));
                memcpy(&bytes[9], &pitch, sizeof(float));
                memcpy(&bytes[13], &yaw, sizeof(float));

                bytes[17] = CRC8(&bytes[3], 14);

                return 18;
            }

            static uint8_t serialize_ALTITUDE_METERS_Request(uint8_t bytes[])
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 60;
                bytes[3] = 0;
                bytes[4] = 123;
                bytes[5] = 123;

                return 6;
            }

            static uint8_t serialize_ALTITUDE_METERS(uint8_t bytes[], float  estalt, float  vario)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 8;
                bytes[4] = 123;

                memcpy(&bytes[5], &estalt, sizeof(float));
                memcpy(&bytes[9], &vario, sizeof(float));

                bytes[13] = CRC8(&bytes[3], 10);

                return 14;
            }

            static uint8_t serialize_RANGE_AND_FLOW_Request(uint8_t bytes[])
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 60;
                bytes[3] = 0;
                bytes[4] = 126;
                bytes[5] = 126;

                return 6;
            }

            static uint8_t serialize_RANGE_AND_FLOW(uint8_t bytes[], int16_t  range, int16_t  flowx, int16_t  flowy)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 6;
                bytes[4] = 126;

                memcpy(&bytes[5], &range, sizeof(int16_t));
                memcpy(&bytes[7], &flowx, sizeof(int16_t));
                memcpy(&bytes[9], &flowy, sizeof(int16_t));

                bytes[11] = CRC8(&bytes[3], 8);

                return 12;
            }

            static uint8_t serialize_SET_MOTOR_NORMAL(uint8_t bytes[], float  m1, float  m2, float  m3, float  m4)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 16;
                bytes[4] = 215;

                memcpy(&bytes[5], &m1, sizeof(float));
                memcpy(&bytes[9], &m2, sizeof(float));
                memcpy(&bytes[13], &m3, sizeof(float));
                memcpy(&bytes[17], &m4, sizeof(float));

                bytes[21] = CRC8(&bytes[3], 18);

                return 22;
            }

            static uint8_t serialize_SET_ARMED(uint8_t bytes[], uint8_t  flag)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 1;
                bytes[4] = 216;

                memcpy(&bytes[5], &flag, sizeof(uint8_t));

                bytes[6] = CRC8(&bytes[3], 3);

                return 7;
            }

            static uint8_t serialize_SET_RC_NORMAL(uint8_t bytes[], float  c1, float  c2, float  c3, float  c4, float  c5, float  c6)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 24;
                bytes[4] = 222;

                memcpy(&bytes[5], &c1, sizeof(float));
                memcpy(&bytes[9], &c2, sizeof(float));
                memcpy(&bytes[13], &c3, sizeof(float));
                memcpy(&bytes[17], &c4, sizeof(float));
                memcpy(&bytes[21], &c5, sizeof(float));
                memcpy(&bytes[25], &c6, sizeof(float));

                bytes[29] = CRC8(&bytes[3], 26);

                return 30;
            }

            static uint8_t serialize_SET_RANGE_AND_FLOW(uint8_t bytes[], int16_t  range, int16_t  flowx, int16_t  flowy)
            {
                bytes[0] = 36;
                bytes[1] = 77;
                bytes[2] = 62;
                bytes[3] = 6;
                bytes[4] = 226;

                memcpy(&bytes[5], &range, sizeof(int16_t));
                memcpy(&bytes[7], &flowx, sizeof(int16_t));
                memcpy(&bytes[9], &flowy, sizeof(int16_t));

                bytes[11] = CRC8(&bytes[3], 8);

                return 12;
            }

    }; // class MspParser

} // namespace hf