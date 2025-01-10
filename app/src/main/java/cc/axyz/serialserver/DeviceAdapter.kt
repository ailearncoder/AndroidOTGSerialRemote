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
import android.text.Html
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import cc.axyz.serialserver.databinding.ItemDeviceBinding
import java.util.Locale

class DeviceAdapter(private val devices: MutableList<Serial.Device>, private val callback: Callback) :
    RecyclerView.Adapter<DeviceAdapter.DeviceViewHolder>() {

    class DeviceViewHolder(val binding: ItemDeviceBinding) : RecyclerView.ViewHolder(binding.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): DeviceViewHolder {
        val binding = ItemDeviceBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return DeviceViewHolder(binding)
    }

    @SuppressLint("SetTextI18n")
    override fun onBindViewHolder(holder: DeviceViewHolder, position: Int) {
        val device = devices[position]
        var deviceName = ""
        if (device.isConnected) {
            deviceName = "${device.driverName} (${Serial.context.getString(R.string.connected)})"
        } else {
            deviceName = device.driverName
        }
        var text =
            String.format(
                Locale.US,
                "Vendor %04X, Product %04X",
                device.vendorId,
                device.productId
            )
        if (device.driverName.isEmpty()) {
            deviceName = "<strike>${Serial.context.getString(R.string.device_not_supported)}</strike>"
            text = "<strike>${text}</strike>"
        }
        holder.binding.deviceNameTextView.text = Html.fromHtml(deviceName, Html.FROM_HTML_MODE_LEGACY)
        holder.binding.deviceInfoTextView.text = Html.fromHtml(text, Html.FROM_HTML_MODE_LEGACY)
    }

    override fun getItemCount(): Int = devices.size

    interface Callback {
        fun onCallback(position: Int)
    }
}
