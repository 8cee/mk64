package com.eightcee.mk64

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.WindowInsets
import android.view.WindowInsetsController
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import java.io.File

class MainActivity : Activity() {
    companion object {
        private const val ROM_PICKER_REQUEST = 64
        init { System.loadLibrary("mk64_android") }
    }

    private external fun nativeRuntimeVersion(): Int
    private external fun nativeRuntimeStage(): Int
    private external fun nativeSetRomPath(path: String): Boolean
    private external fun nativeInitPlatform(): Boolean
    private external fun nativeAdvanceFrame(): Int
    private external fun nativeResumeClock()
    private external fun nativeSetController(buttons: Int, stickX: Int, stickY: Int)

    private var controllerButtons = 0
    private var controllerStickX = 0
    private var controllerStickY = 0

    private lateinit var status: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.insetsController?.let {
            it.hide(WindowInsets.Type.systemBars())
            it.systemBarsBehavior = WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        nativeInitPlatform()

        val root = FrameLayout(this)
        val gameSurface = Mk64Surface(this)
        root.addView(gameSurface, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ))
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 32, 32, 32)
        }
        status = TextView(this).apply {
            text = "MK64 Android\nNative runtime v${nativeRuntimeVersion()} · stage ${nativeRuntimeStage()}"
            textSize = 22f
        }
        val selectRom = Button(this).apply {
            text = "Select Mario Kart 64 ROM"
            setOnClickListener { openRomPicker() }
        }
        panel.addView(status)
        panel.addView(selectRom)
        root.addView(panel)
        val touchControls = TouchControlsView(this) { buttons, x, y ->
            controllerButtons = buttons
            controllerStickX = x
            controllerStickY = y
            pushController()
        }
        root.addView(touchControls, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ))
        panel.bringToFront()
        setContentView(root)
    }

    override fun onResume() {
        super.onResume()
        nativeResumeClock()
    }

    override fun onPause() {
        super.onPause()
    }


    private fun pushController() {
        nativeSetController(controllerButtons, controllerStickX, controllerStickY)
    }

    private fun n64ButtonFor(keyCode: Int): Int = when (keyCode) {
        KeyEvent.KEYCODE_BUTTON_A -> 0x8000
        KeyEvent.KEYCODE_BUTTON_B -> 0x4000
        KeyEvent.KEYCODE_BUTTON_L2 -> 0x2000
        KeyEvent.KEYCODE_BUTTON_START -> 0x1000
        KeyEvent.KEYCODE_DPAD_UP -> 0x0800
        KeyEvent.KEYCODE_DPAD_DOWN -> 0x0400
        KeyEvent.KEYCODE_DPAD_LEFT -> 0x0200
        KeyEvent.KEYCODE_DPAD_RIGHT -> 0x0100
        KeyEvent.KEYCODE_BUTTON_L1 -> 0x0020
        KeyEvent.KEYCODE_BUTTON_R1 -> 0x0010
        KeyEvent.KEYCODE_BUTTON_Y -> 0x0008
        KeyEvent.KEYCODE_BUTTON_X -> 0x0004
        KeyEvent.KEYCODE_BUTTON_SELECT -> 0x0002
        KeyEvent.KEYCODE_BUTTON_R2 -> 0x0001
        else -> 0
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        if (event.source and (InputDevice.SOURCE_GAMEPAD or InputDevice.SOURCE_JOYSTICK) != 0) {
            val mask = n64ButtonFor(event.keyCode)
            if (mask != 0) {
                controllerButtons = if (event.action == KeyEvent.ACTION_DOWN) {
                    controllerButtons or mask
                } else {
                    controllerButtons and mask.inv()
                }
                pushController()
                return true
            }
        }
        return super.dispatchKeyEvent(event)
    }

    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        if (event.source and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK &&
            event.action == MotionEvent.ACTION_MOVE) {
            fun axis(axis: Int): Float {
                val range = event.device?.getMotionRange(axis, event.source)
                val value = event.getAxisValue(axis)
                return if (range != null && kotlin.math.abs(value) <= range.flat) 0f else value
            }
            controllerStickX = (axis(MotionEvent.AXIS_X).coerceIn(-1f, 1f) * 80f).toInt()
            controllerStickY = (-axis(MotionEvent.AXIS_Y).coerceIn(-1f, 1f) * 80f).toInt()
            pushController()
            return true
        }
        return super.onGenericMotionEvent(event)
    }

    private fun openRomPicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/octet-stream"
            putExtra(Intent.EXTRA_MIME_TYPES, arrayOf("application/octet-stream", "application/x-n64-rom", "*/*"))
        }
        startActivityForResult(intent, ROM_PICKER_REQUEST)
    }

    @Deprecated("Deprecated in Android API; kept for the simple document-picker bridge.")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != ROM_PICKER_REQUEST || resultCode != RESULT_OK) return
        val uri = data?.data ?: return
        try {
            contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
        } catch (_: SecurityException) {
            // Some document providers grant only temporary access; copying below still works.
        }
        importRom(uri)
    }

    private fun importRom(uri: Uri) {
        val romDir = File(filesDir, "rom").apply { mkdirs() }
        val target = File(romDir, "mk64.z64")
        try {
            contentResolver.openInputStream(uri)?.use { input ->
                target.outputStream().use { output -> input.copyTo(output) }
            } ?: error("Unable to open selected ROM")
            val accepted = nativeSetRomPath(target.absolutePath)
            status.text = if (accepted) {
                "ROM imported\n${target.name} · ${target.length()} bytes · stage ${nativeRuntimeStage()}"
            } else {
                "ROM import failed in native runtime"
            }
        } catch (t: Throwable) {
            status.text = "ROM import failed: ${t.message ?: t.javaClass.simpleName}"
        }
    }
}
