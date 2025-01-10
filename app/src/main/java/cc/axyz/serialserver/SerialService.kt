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

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.drawable.Icon
import android.hardware.usb.UsbManager
import android.os.Build
import android.os.IBinder
import android.os.PowerManager
import android.os.PowerManager.WakeLock
import android.system.ErrnoException
import android.system.Os
import android.util.Log
import android.widget.Toast


class SerialService : Service() {
    var toast: Toast? = null

    private var init = false
    private val notificationId = "串口服务"
    private val notificationName = "串口服务通知"
    private var notificationManager: NotificationManager? = null
    private var wakeLock: WakeLock? = null
    private var broadcastReceiver: BroadcastReceiver? = null
    private var notificationMessage: String = "运行中..."
    val TAG: String = "SerialService"

    private fun getNotification(ticker: String, main: Boolean): Notification {
        val builder: Notification.Builder = Notification.Builder(this)
            .setSmallIcon(R.mipmap.ic_launcher)
            .setContentTitle("串口服务")
            .setContentText(ticker)
            .setTicker(ticker)
        //设置Notification的ChannelID,否则不能正常显示
        builder.setChannelId(notificationId)
        if (main) {
            run {
                val intent1 = Intent()
                intent1.setAction("SerialService")
                if (!wakeLock?.isHeld!!) {
                    intent1.putExtra("action", "lock")
                    val intent = PendingIntent.getBroadcast(
                        applicationContext,
                        0x01,
                        intent1,
                        PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
                    )
                    val lockAction = Notification.Action.Builder(
                        Icon.createWithResource(this, R.mipmap.ic_launcher),
                        "锁定",
                        intent
                    ).build()
                    builder.addAction(lockAction)
                } else {
                    intent1.putExtra("action", "unlock")
                    val intent = PendingIntent.getBroadcast(
                        applicationContext,
                        0x01,
                        intent1,
                        PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
                    )
                    val unlockAction = Notification.Action.Builder(
                        Icon.createWithResource(this, R.mipmap.ic_launcher),
                        "解锁",
                        intent
                    ).build()
                    builder.addAction(unlockAction)
                }
            }
            run {
                val intent1 = Intent()
                intent1.putExtra("action", "exit")
                intent1.setAction("SerialService")
                val intent = PendingIntent.getBroadcast(
                    applicationContext,
                    0x02,
                    intent1,
                    PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
                )
                val exitAction = Notification.Action.Builder(
                    Icon.createWithResource(this, R.mipmap.ic_launcher),
                    "退出",
                    intent
                ).build()
                builder.addAction(exitAction)
            }
        }
        // notification点击打开Activity
        val intent = Intent(this, MainActivity::class.java)
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK)
        builder.setContentIntent(PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE))
        val notification = builder.build()
        return notification
    }

    private fun getNotification(ticker: String): Notification {
        return getNotification(ticker, false)
    }

    override fun onCreate() {
        super.onCreate()
//        val filter = IntentFilter()
//        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
//        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
//        filter.addAction(Serial.INTENT_ACTION_GRANT_USB)
//        registerReceiver(usbDetachedReceiver, filter, RECEIVER_NOT_EXPORTED)
        val pm = getSystemService(POWER_SERVICE) as PowerManager
        wakeLock = pm.newWakeLock(
            PowerManager.ON_AFTER_RELEASE or PowerManager.PARTIAL_WAKE_LOCK,
            "SerialService:Tag"
        )
        wakeLock?.acquire()
        broadcastReceiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                Log.d(TAG, "onReceive: " + intent.action)
                if (intent.action == "SerialService") {
                    val action = intent.getStringExtra("action")
                    if ("exit" == action) {
                        try {
                            Os.kill(Os.getpid(), 9)
                        } catch (e: ErrnoException) {
                            e.printStackTrace()
                        }
                    }
                    if ("lock" == action) {
                        if (!wakeLock?.isHeld!!) wakeLock?.acquire()
                        notificationManager!!.notify(1, getNotification(notificationMessage, true))
                    }
                    if ("unlock" == action) {
                        if (wakeLock?.isHeld!!) wakeLock?.release()
                        notificationManager!!.notify(1, getNotification(notificationMessage, true))
                    }
                }
            }
        }
        registerReceiver(broadcastReceiver, IntentFilter("SerialService"), RECEIVER_NOT_EXPORTED)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        notificationManager = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
        //创建NotificationChannel
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                notificationId,
                notificationName,
                NotificationManager.IMPORTANCE_HIGH
            )
            notificationManager!!.createNotificationChannel(channel)
        }
        var defaultMessage = getString(R.string.no_usb_device)
        val usbManager = getSystemService(Context.USB_SERVICE) as UsbManager
        if (usbManager.deviceList.size > 0) {
            defaultMessage = getString(R.string.usb_device_connected)
        }
        notificationMessage = intent?.getStringExtra("message") ?: defaultMessage
        startForeground(1, getNotification(notificationMessage, true))
        if (!init) {
            init = true
            Thread {
                rfc2217Init(applicationInfo.nativeLibraryDir)
                while (true) {
                    rfc2217Start(-1, 2217, 2)
                    notificationMessage = getString(R.string.service_rebooting)
                    startForeground(1, getNotification(notificationMessage, true))
                    Thread.sleep(1000)
                }
            }.start()
        }
        return START_STICKY
    }

    override fun onDestroy() {
//        unregisterReceiver(usbDetachedReceiver)
        super.onDestroy()
    }

    override fun onBind(intent: Intent): IBinder? {
        return null
    }

    // 1. 创建BroadcastReceiver
    private val usbDetachedReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            toast?.cancel()
            val action = intent.action
            if (UsbManager.ACTION_USB_DEVICE_DETACHED == action) {
                // USB设备被拔出，执行相应操作
                // 这里可以根据需要处理设备拔出的逻辑
                toast = Toast.makeText(context, "USB设备已拔出", Toast.LENGTH_SHORT)
                toast?.show()
            }
            if (UsbManager.ACTION_USB_DEVICE_ATTACHED == action) {
                // USB设备被拔出，执行相应操作
                // 这里可以根据需要处理设备拔出的逻辑
                toast = Toast.makeText(context, "USB设备已插入", Toast.LENGTH_SHORT)
                toast?.show()
            }
            if (Serial.INTENT_ACTION_GRANT_USB == action) {
                val usbPermission =
                    intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                Log.d(Companion.TAG, "usbPermission: $usbPermission")
            }
        }
    }


    companion object {
        // Used to load the 'serialserver' library on application startup.
        init {
            System.loadLibrary("serialserver")
        }
        private const val TAG = "SerialService"
        /**
         * A native method that is implemented by the 'serialserver' native library,
         * which is packaged with this application.
         */
        @JvmStatic
        external fun stringFromJNI(): String
        @JvmStatic
        external fun rfc2217Init( binaryFilename:String)
        @JvmStatic
        external fun rfc2217Start( port:Int, tcpPort:Int, verbose:Int)
    }
}
