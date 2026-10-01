package com.eightcee.mk64

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.WindowInsets
import android.view.WindowInsetsController
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import java.io.File

class MainActivity : Activity() {
    companion object {
        private const val ROM_PICKER_REQUEST = 64
    }

    private external fun nativeRuntimeVersion(): Int
    private external fun nativeRuntimeStage(): Int
    private external fun nativeSetRomPath(path: String): Boolean
    private external fun nativeLoadAssets(): Boolean
    private external fun nativeInitGame(): Boolean
    private external fun nativeInitPlatform(): Boolean
    private external fun nativeResumeClock()
    private external fun nativeSetController(buttons: Int, stickX: Int, stickY: Int)

    private var nativeLoaded = false
    private var platformReady = false
    private var gameReady = false
    private var controllerButtons = 0
    private var controllerStickX = 0
    private var controllerStickY = 0
    private lateinit var status: TextView
    private lateinit var root: FrameLayout

    private val prefs by lazy { getSharedPreferences("mk64_state", MODE_PRIVATE) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        hideSystemUi()

        if (!loadNativeRuntime()) {
            showFatal("MK64 could not start.")
            return
        }

        val rememberedRom = File(filesDir, "rom/mk64.z64")
        if (rememberedRom.isFile && rememberedRom.length() >= 8L * 1024L * 1024L) {
            showLauncher("Starting Mario Kart 64…", false)
            bootRom(rememberedRom)
        } else {
            showLauncher("Choose your Mario Kart 64 (USA) ROM to start.", true)
        }
    }

    private fun hideSystemUi() {
        window.decorView.systemUiVisibility =
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE or
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_FULLSCREEN

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R) {
            window.insetsController?.let {
                it.hide(WindowInsets.Type.systemBars())
                it.systemBarsBehavior = WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            }
        }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) hideSystemUi()
    }

    private fun mark(stage: String) {
        prefs.edit().putString("last_stage", stage).apply()
    }

    private fun loadNativeRuntime(): Boolean {
        return try {
            mark("load_library")
            System.loadLibrary("mk64_android")
            nativeLoaded = true

            mark("platform_init")
            platformReady = nativeInitPlatform()
            platformReady
        } catch (_: Throwable) {
            false
        }
    }

    private fun showLauncher(message: String, allowSelect: Boolean) {
        root = FrameLayout(this)
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(48, 48, 48, 48)
        }

        status = TextView(this).apply {
            text = "Mario Kart 64\n\n$message"
            textSize = 22f
        }
        panel.addView(status)

        if (allowSelect) {
            panel.addView(Button(this).apply {
                text = "Select ROM"
                setOnClickListener { openRomPicker() }
            })
        }

        root.addView(panel)
        setContentView(root)
    }

    private fun bootRom(target: File) {
        try {
            mark("rom_validate")
            if (!nativeSetRomPath(target.absolutePath)) {
                showLauncher("That ROM is not the supported Mario Kart 64 USA image.", true)
                return
            }

            mark("asset_load")
            if (!nativeLoadAssets()) {
                showLauncher("Could not prepare Mario Kart 64 game data.", true)
                return
            }

            /*
             * Do not initialize the MK64 renderer here. This method runs on the
             * Android UI thread before GLSurfaceView owns a current EGL context.
             * The game/renderer is initialized from Mk64Surface.onSurfaceCreated()
             * on the GL thread instead.
             */
            mark("surface_pending")
            startGameUi()
        } catch (_: Throwable) {
            showLauncher("Mario Kart 64 could not start with this ROM.", true)
        }
    }

    private fun startGameUi() {
        val gameRoot = FrameLayout(this)
        val gameSurface = Mk64Surface(this)
        gameRoot.addView(
            gameSurface,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            )
        )

        val touchControls = TouchControlsView(this) { buttons, x, y ->
            controllerButtons = buttons
            controllerStickX = x
            controllerStickY = y
            pushController()
        }
        gameRoot.addView(
            touchControls,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            )
        )

        root = gameRoot
        setContentView(root)
        hideSystemUi()
    }

    private fun showFatal(message: String) {
        setContentView(TextView(this).apply {
            text = message
            textSize = 20f
            setPadding(48, 48, 48, 48)
        })
    }

    override fun onResume() {
        super.onResume()
        hideSystemUi()
        if (nativeLoaded && platformReady) {
            try { nativeResumeClock() } catch (_: Throwable) {}
        }
    }

    private fun pushController() {
        if (nativeLoaded && platformReady) {
            try {
                nativeSetController(controllerButtons, controllerStickX, controllerStickY)
            } catch (_: Throwable) {
            }
        }
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
        if (nativeLoaded &&
            event.source and (InputDevice.SOURCE_GAMEPAD or InputDevice.SOURCE_JOYSTICK) != 0
        ) {
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
        if (nativeLoaded &&
            event.source and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK &&
            event.action == MotionEvent.ACTION_MOVE
        ) {
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
            type = "*/*"
            putExtra(Intent.EXTRA_MIME_TYPES, arrayOf("application/octet-stream", "application/x-n64-rom"))
        }
        startActivityForResult(intent, ROM_PICKER_REQUEST)
    }

    @Deprecated("Kept for the Android document picker bridge.")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != ROM_PICKER_REQUEST || resultCode != RESULT_OK) return
        val uri = data?.data ?: return
        importRom(uri)
    }

    private fun importRom(uri: Uri) {
        val romDir = File(filesDir, "rom").apply { mkdirs() }
        val target = File(romDir, "mk64.z64")
        val temp = File(romDir, "mk64.z64.tmp")

        try {
            contentResolver.openInputStream(uri)?.use { input ->
                temp.outputStream().use { output ->
                    val buffer = ByteArray(64 * 1024)
                    var total = 0L
                    while (true) {
                        val read = input.read(buffer)
                        if (read < 0) break
                        total += read
                        if (total > 64L * 1024L * 1024L) {
                            error("ROM is too large")
                        }
                        output.write(buffer, 0, read)
                    }
                }
            } ?: error("Unable to open selected ROM")

            if (target.exists()) target.delete()
            if (!temp.renameTo(target)) {
                temp.copyTo(target, overwrite = true)
                temp.delete()
            }

            showLauncher("Starting Mario Kart 64…", false)
            bootRom(target)
        } catch (_: Throwable) {
            temp.delete()
            showLauncher("Could not import the selected ROM.", true)
        }
    }
}
