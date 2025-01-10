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

import android.app.Application
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbManager
import android.util.Log
import android.widget.Toast

class SerialApplication: Application() {

    private val TAG = "SerialApplication"
    private var toast: Toast? = null

    override fun onCreate() {
        super.onCreate()
        Serial.context = applicationContext
        val filter = IntentFilter()
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
        filter.addAction(Serial.INTENT_ACTION_GRANT_USB)
        registerReceiver(usbDetachedReceiver, filter, RECEIVER_NOT_EXPORTED)
    }

    override fun onTerminate() {
        super.onTerminate()
        unregisterReceiver(usbDetachedReceiver)
    }

    private val usbDetachedReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            toast?.cancel()
            val action = intent.action
            if (UsbManager.ACTION_USB_DEVICE_DETACHED == action) {
                // USB设备被拔出，执行相应操作
                // 这里可以根据需要处理设备拔出的逻辑
                toast = Toast.makeText(context, "USB设备已拔出", Toast.LENGTH_SHORT)
                toast?.show()
                MainActivity.updateDeviceList()
                val serviceIntent = Intent(applicationContext, SerialService::class.java)
                startForegroundService(serviceIntent)
                Serial.usbStateChanged()
                // callHomeFragmentMethod()
            }
            if (UsbManager.ACTION_USB_DEVICE_ATTACHED == action) {
                // USB设备被拔出，执行相应操作
                // 这里可以根据需要处理设备拔出的逻辑
                toast = Toast.makeText(context, "USB设备已插入", Toast.LENGTH_SHORT)
                toast?.show()
                MainActivity.updateDeviceList()
                val serviceIntent = Intent(applicationContext, SerialService::class.java)
                startForegroundService(serviceIntent)
                // callHomeFragmentMethod()
            }
            if (Serial.INTENT_ACTION_GRANT_USB == action) {
                val usbPermission =
                    intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                Log.d(TAG, "usbPermission: $usbPermission")
                Serial.setPermission(usbPermission)
                // callback?.onPermissionCallback(usbPermission)
                // callback = null
            }
        }
    }
}
