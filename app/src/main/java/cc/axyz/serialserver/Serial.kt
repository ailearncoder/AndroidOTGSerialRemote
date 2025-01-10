/*
 * MIT License
 *
 * Copyright (c) 2025 by ailearncoder <panxuesen520@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

package cc.axyz.serialserver

import android.annotation.SuppressLint
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
import android.util.Log
import com.hoho.android.usbserial.BuildConfig
import com.hoho.android.usbserial.driver.UsbSerialPort
import com.hoho.android.usbserial.driver.UsbSerialProber
import com.hoho.android.usbserial.util.SerialInputOutputManager
import org.json.JSONArray
import org.json.JSONObject
import java.util.concurrent.Semaphore
import java.util.concurrent.TimeUnit
import java.util.concurrent.locks.Condition
import java.util.concurrent.locks.ReentrantLock


class Serial {
    data class Device (
        var deviceId: Int = -1,
        var driverName: String = "",
        var vendorId: Int = -1,
        var productId: Int = -1,
        var isConnected: Boolean = false
    )
    data class SerialInstance(
        var baudRate: Int = -1,
        var dataBits: Int = -1,
        var stopBits: Float = -1.0f,
        var parity: Char = 'N',
        var port: UsbSerialPort? = null,
        var data: ArrayList<Byte> = arrayListOf(),
        val lock : ReentrantLock = ReentrantLock(),
        val notEmpty: Condition = lock.newCondition(),
        var usbIoManager : SerialInputOutputManager? = null,
        var info: String = "",
        var deviceId: Int = -1
    )
    
    class SerialInputOutputManagerListener(private val serialInstance: SerialInstance) : SerialInputOutputManager.Listener {
        override fun onNewData(data: ByteArray) {
            // Handle the new incoming data
            Log.d(TAG, "onNewData: Received new data ${data.size} bytes")
            // 这里可以添加你的数据处理逻辑
            serialInstance.lock.lock()
            try {
                val buffer = serialInstance.data
                for (byte in data) {
                    buffer.add(byte)
                }
                serialInstance.notEmpty.signalAll()
            } finally {
                serialInstance.lock.unlock()
            }
        }

        override fun onRunError(e: java.lang.Exception?) {
            Log.e(TAG, "${serialInstance.info} Error in SerialInputOutputManager: $e")
        }
    }
    
    companion object {
        private const val TAG = "Serial"
        const val INTENT_ACTION_GRANT_USB: String = BuildConfig.LIBRARY_PACKAGE_NAME + ".GRANT_USB"

        @SuppressLint("StaticFieldLeak")
        lateinit var context: Context
        private val usbSerialInstances: HashMap<Int, SerialInstance> = HashMap()
        private val usbSerialSet = HashSet<Int>()
        private var permission = false
        private val signalPermission = Semaphore(0)
        private var classLoader: ClassLoader? = Serial::class.java.getClassLoader()
        private val lock = Any()
        
        private fun usbSerialAdd(id: Int, serialInstance: SerialInstance) {
            synchronized(lock) {
                usbSerialInstances[id] = serialInstance
                usbSerialSet.add(serialInstance.deviceId)
                MainActivity.updateDeviceList()
            }
        }
        
        private fun usbSerialGet(id: Int) : SerialInstance? {
            synchronized(lock) {
                return usbSerialInstances[id]
            }
        }
        
        private fun usbSerialRemove(id: Int) {
            synchronized(lock) {
                usbSerialSet.remove(usbSerialInstances[id]?.deviceId ?: -1)
                usbSerialInstances.remove(id)
                MainActivity.updateDeviceList()
            }
        }

        private fun serviceNotify(message: String) {
            val serviceIntent = Intent(context, SerialService::class.java)
            serviceIntent.putExtra("message", message)
            context.startForegroundService(serviceIntent)
        }

        private fun printBytes(msg: String, bytes: ArrayList<Byte>?) {
            if (bytes == null) return
            val stringBuilder = StringBuilder()
            if (bytes.size <= 32) {
                for (i in 0 until bytes.size) {
                    stringBuilder.append(String.format("%02X ", bytes[i]))
                }
            } else {
                for (i in 0 until 16) {
                    stringBuilder.append(String.format("%02X ", bytes[i]))
                }
                stringBuilder.append("... ")
                for (i in bytes.size - 16 until bytes.size) {
                    stringBuilder.append(String.format("%02X ", bytes[i]))
                }
            }
            Log.d(TAG, "$msg: $stringBuilder")
        }

        fun usbStateChanged() {
            val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
            val deviceSets = HashSet<Int>()
            for (device in usbManager.getDeviceList().values) {
                deviceSets.add(device.deviceId)
            }
            synchronized(lock) {
                val iterator = usbSerialInstances.entries.iterator()
                while (iterator.hasNext()) {
                    val entry = iterator.next()
                    // Check if the device is still connected.
                    if (!deviceSets.contains(entry.value.deviceId)) {
                        entry.value.port?.close()
                        entry.value.usbIoManager?.stop()
                        iterator.remove()
                        usbSerialSet.remove(entry.value.deviceId)
                        Log.d(TAG, "Device disconnected: ${entry.value.info}")
                    }
                }
            }
        }

        fun setPermission(permission: Boolean) {
            signalPermission.release()
            this.permission = permission
        }

        fun listSerial() : String {
            val jsonObject = JSONObject()
            val jsonList = JSONArray()
            // Open the first available driver.
            val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
            val usbDefaultProbe = UsbSerialProber.getDefaultProber()
            for (device in usbManager.getDeviceList().values) {
                val id = device.deviceId
                val vendor = device.vendorId
                val product = device.productId
                val driver = usbDefaultProbe.probeDevice(device)
                val driverName = driver::class.simpleName?.replace("SerialDriver", "")
                val json = JSONObject()
                json.put("id", id)
                json.put("vendorId", vendor)
                json.put("productId", product)
                json.put("driverName", driverName ?:"Unknown")
                jsonList.put(json)
            }
            jsonObject.put("list", jsonList)
            return jsonObject.toString()
        }

        fun getDevices() : ArrayList<Device> {
            val list = ArrayList<Device>()
            val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
            val usbDefaultProbe = UsbSerialProber.getDefaultProber()
            for (device in usbManager.getDeviceList().values) {
                val driver = usbDefaultProbe.probeDevice(device)
                var driverName = ""
                if (driver != null) {
                    driverName = driver::class.simpleName?.replace("SerialDriver", "").toString()
                }
                val serialDevice = Device()
                serialDevice.deviceId = device.deviceId
                serialDevice.vendorId = device.vendorId
                serialDevice.productId = device.productId
                serialDevice.driverName = driverName
                synchronized(lock) {
                    serialDevice.isConnected = usbSerialSet.contains(device.deviceId)
                }
                list.add(serialDevice)
            }
            return list
        }

        private fun idExist(id: Int) : Boolean {
            val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
            for (device in usbManager.getDeviceList().values) {
                if(id == device.deviceId) return true
            }
            return false
        }

        fun requestSerial(id: Int) : Int {
            val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
            for (device in usbManager.getDeviceList().values) {
                if(id == device.deviceId) {
                    val flags = PendingIntent.FLAG_MUTABLE
                    val intent = Intent(INTENT_ACTION_GRANT_USB)
                    intent.setPackage(context.packageName)
                    val usbPermissionIntent = PendingIntent.getBroadcast(context, 0x01, intent, flags)
                    usbManager.requestPermission(device, usbPermissionIntent)
                    return 1
                }
            }
            return 0
        }

        @JvmStatic
        fun getClassLoader(): ClassLoader? {
            return classLoader
        }

        @JvmStatic
        fun openSerial(id: Int): Int {
            val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
            val usbDefaultProbe = UsbSerialProber.getDefaultProber()

            for (device in usbManager.getDeviceList().values) {
                if (id == device.deviceId || id == 0) {
                    permission = false
                    if (!usbManager.hasPermission(device)) {
                        Log.d(TAG, "openSerial: Requesting permission for device $id") // 添加日志
                        requestSerial(device.deviceId)
                        signalPermission.tryAcquire(10, TimeUnit.SECONDS)
                        if (!permission) {
                            Log.w(TAG, "openSerial: Permission request timed out for device $id") // 添加日志
                            return 0
                        }
                    }
                    val driver = usbDefaultProbe.probeDevice(device)
                    if (driver != null) {
                        val port = driver.ports[0]
                        val connection: UsbDeviceConnection = usbManager.openDevice(driver.device)
                        port.open(connection)
                        val instance = SerialInstance()
                        instance.port = port
                        val listener = SerialInputOutputManagerListener(instance)
                        instance.usbIoManager = SerialInputOutputManager(port, listener)
                        val driverName = driver::class.simpleName?.replace("SerialDriver", "")
                        instance.info = String.format("%s %X:%X", driverName, device.vendorId, device.productId)
                        instance.deviceId = device.deviceId
                        serviceNotify("串口打开: ${instance.info}")
                        instance.usbIoManager!!.start()
                        usbSerialAdd(id, instance)
                        Log.i(TAG, "openSerial: Successfully opened serial port for device $id") // 添加日志
                        return 1
                    }
                }
            }
            Log.e(TAG, "openSerial: No suitable serial port found for device $id") // 添加日志
            return -1
        }

        @JvmStatic
        fun closeSerial(id: Int): Int {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.d(TAG, "closeSerial: Port with id $id not found.")
                return 0
            }
            serviceNotify("串口关闭: ${instance.info}")
            instance.port?.close()
            instance.usbIoManager?.stop()
            usbSerialRemove(id)
            if (!idExist(id)) {
                Log.d(TAG, "closeSerial: Port with id $id removed due to non-existence.")
                return 0
            }
            Log.d(TAG, "closeSerial: Port with id $id closed and removed.")
            return 1
        }

        @JvmStatic
        fun configureSerial(id: Int, baudRate: Int, dataBits: Int, stopBits: Float, parity: Char) : Int {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.d(TAG, "configureSerial: Port not found for ID $id")
                return 0
            }
            val portStopBits = when (stopBits) {
                1f -> UsbSerialPort.STOPBITS_1
                1.5f -> UsbSerialPort.STOPBITS_1_5
                else -> UsbSerialPort.STOPBITS_2
            }
            val portParity = when (parity) {
                'N' -> UsbSerialPort.PARITY_NONE
                'E' -> UsbSerialPort.PARITY_EVEN
                else -> UsbSerialPort.PARITY_ODD
            }
            instance.port?.setParameters(baudRate, dataBits, portStopBits, portParity)
            Log.d(TAG, "configureSerial: Port $id configured successfully with baudRate=$baudRate, dataBits=$dataBits, stopBits=$stopBits, parity=$parity")
            return 1
        }

        @JvmStatic
        fun readSerial(id: Int, size: Int, timeout: Int): ByteArray? {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.e(TAG, "readSerial: Port ID $id is invalid")
                return null
            }
            val buffer = ByteArray(size)
            Log.d(TAG, "readSerial: Attempting to read $size bytes from port $id with timeout=$timeout")

            var len = 0
            instance.lock.lock()
            try {
                while (len < size) {
                    if (instance.data.isEmpty()) {
                        if (!instance.notEmpty.await(timeout.toLong(), TimeUnit.MILLISECONDS)) {
                            Log.e(TAG, "readSerial: Timeout reached while reading from port $id")
                            break
                        }
                    } else {
                        buffer[len] = instance.data.removeAt(0)
                        len++
                    }
                }
            } catch (e: InterruptedException) {
                Log.e(TAG, "readSerial: Interrupted while waiting for data on port $id: $e")
            } finally {
                instance.lock.unlock()
            }

            return if (len == 0) {
                Log.e(TAG, "readSerial: Failed to read from port $id")
                byteArrayOf()
            } else {
                Log.d(TAG, "readSerial: Successfully read $len bytes from port $id")
                buffer.copyOf(len)
            }
        }
    
        @JvmStatic
        fun writeSerial(id: Int, data : ByteArray, timeout: Int) : Int {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.e(TAG, "writeSerial: Port ID $id is invalid")
                return -1
            }
            instance.port?.write(data, data.size, timeout)
            Log.d(TAG, "writeSerial: Successfully wrote ${data.size} bytes to port $id with timeout=$timeout")
            return 0
        }

        @JvmStatic
        fun rtsSerialSet(id: Int, state: Boolean) : Int {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.e(TAG, "rtsSerialSet: Port ID $id is invalid")
                return -1
            }
            instance.port?.rts = state
            Log.d(TAG, "rtsSerialSet: Set RTS to $state for port $id")
            return 0
        }
        @JvmStatic
        fun rtsSerialGet(id: Int) : Boolean {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.e(TAG, "rtsSerialGet: Port ID $id is invalid")
                return false
            }
            val res =  instance.port?.rts ?: false
            Log.d(TAG, "rtsSerialGet: RTS state for port $id is $res")
            return res
        }
        @JvmStatic
        fun dtrSerialSet(id: Int, state: Boolean) : Int {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.e(TAG, "dtrSerialSet: Port ID $id is invalid")
                return -1
            }
            instance.port?.dtr = state
            Log.d(TAG, "dtrSerialSet: Set DTR to $state for port $id")
            return 0
        }
        @JvmStatic
        fun dtrSerialGet(id: Int) : Boolean {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.e(TAG, "dtrSerialGet: Port ID $id is invalid")
                return false
            }
            val res = instance.port?.dtr ?: false
            Log.d(TAG, "dtrSerialGet: DTR state for port $id is $res")
            return res
        }
        @JvmStatic
        fun statusSerial(id: Int, name: String) : Int {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.e(TAG, "statusSerial: Port ID $id is invalid")
                return -1
            }
            var res = true
            try {
                // cts, dsr, cd, ri
                res =  when (name) {
                    "cts" -> instance.port?.cts ?: false
                    "dsr" -> instance.port?.dsr ?: false
                    "cd" -> instance.port?.cd ?: false
                    "ri" -> instance.port?.ri ?: false
                    else -> {
                        Log.e(TAG, "statusSerial: Invalid status name $name")
                        false
                    }
                }
                Log.d(TAG, "statusSerial: $name state for port $id is $res")
            } catch (e: UnsupportedOperationException) {
                Log.i(TAG, "statusSerial: Failed to get $name state for port $id")
            }
            return if (res) 1 else 0
        }

        @JvmStatic
        fun inWaitingSerial(id: Int) : Int {
            val res: Int
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.e(TAG, "inWaiting: Port ID $id is invalid")
                return 0
            }
            instance.lock.lock()
            try {
                res = instance.data.size
                Log.d(TAG, "inWaiting: $res bytes in buffer for port $id")
            } finally {
                instance.lock.unlock()
            }
            return res
        }

        @JvmStatic
        fun resetInputBufferSerial(id: Int) : Boolean {
            val instance = usbSerialGet(id)
            if (instance == null) {
                Log.e(TAG, "resetInputBuffer: Port ID $id is invalid")
                return false
            }
            instance.lock.lock()
            try {
                printBytes("resetInputBuffer", instance.data)
                instance.data.clear()
                Log.d(TAG, "resetInputBuffer: Cleared buffer for port $id")
            } finally {
                instance.lock.unlock()
            }
            return true
        }
    }
}
