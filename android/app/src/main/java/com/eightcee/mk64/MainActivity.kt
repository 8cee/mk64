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

    private lateinit var status: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.insetsController?.let {
            it.hide(WindowInsets.Type.systemBars())
            it.systemBarsBehavior = WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        nativeInitPlatform()

        val root = FrameLayout(this)
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
        setContentView(root)
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
