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

import android.Manifest.permission.POST_NOTIFICATIONS
import android.annotation.SuppressLint
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import cc.axyz.serialserver.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var deviceAdapter: DeviceAdapter
    private val listItems: MutableList<Serial.Device> = ArrayList()

    private fun checkAndRequestNotificationPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(
                    this,
                    POST_NOTIFICATIONS
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                Toast.makeText(
                    this,
                    "需要通知权限",
                    Toast.LENGTH_SHORT
                ).show()
                // 显示解释对话框（如果需要），然后请求权限
                ActivityCompat.requestPermissions(
                    this,
                    arrayOf(POST_NOTIFICATIONS),
                    PERMISSION_REQUEST_CODE
                )
            } else {
                // 权限已经被授予，可以发送通知
                startService()
            }
        } else {
            // 对于 Android 13 以下的版本，不需要请求此权限
            // 可以直接发送通知
        }
    }

    private fun startService() {
        val serviceIntent = Intent(this, SerialService::class.java)
        startForegroundService(serviceIntent)
        init = true
    }

    private fun initView() {
        deviceAdapter = DeviceAdapter(listItems, object : DeviceAdapter.Callback {
            override fun onCallback(position: Int) {
            }
        })
        binding.recyclerView.adapter = deviceAdapter
    }

    @SuppressLint("NotifyDataSetChanged")
    fun updateDeviceList() {
        runOnUiThread {
            val devices = Serial.getDevices()
            listItems.clear()
            listItems.addAll(devices)
            deviceAdapter.notifyDataSetChanged()
            if (devices.isEmpty()) {
                binding.sampleText.text = getText(R.string.no_usb_device)
                binding.recyclerView.visibility = View.GONE
            } else {
                binding.sampleText.text = getText(R.string.usb_device_list)
                binding.recyclerView.visibility = View.VISIBLE
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        Log.d("MainActivity", "onCreate: MainActivity ${applicationInfo.nativeLibraryDir}")
        // Example of a call to a native method
        // binding.sampleText.text = SerialService.stringFromJNI()
        initView()
    }

    override fun onResume() {
        super.onResume()
        if (!init) {
            // 第一次启动时，检查通知权限并请求（如果需要）
            checkAndRequestNotificationPermission()
        }
        updateDeviceList()
        synchronized(lock) {
            instance = this
        }
    }

    override fun onPause() {
        super.onPause()
        synchronized(lock) {
            instance = null
        }
    }

    override fun onDestroy() {
        super.onDestroy()
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (resultCode == RESULT_OK) {
                // 用户授予了权限，可以发送通知
                startService()
            } else {
                Toast.makeText(this, "需要通知权限", Toast.LENGTH_SHORT).show()
            }
        }
        super.onActivityResult(requestCode, resultCode, data)
    }

    companion object {
        private var init = false
        const val PERMISSION_REQUEST_CODE = 1 // 你自己定义的请求码
        const val TAG = "MainActivity"
        private var instance: MainActivity? = null
        private var lock = Any()
        fun updateDeviceList() {
            synchronized(lock) {
                instance?.updateDeviceList()
            }
        }
    }
}
