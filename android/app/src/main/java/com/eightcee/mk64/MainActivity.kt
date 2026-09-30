package com.eightcee.mk64

import android.app.Activity
import android.os.Bundle
import android.view.WindowInsets
import android.view.WindowInsetsController
import android.widget.FrameLayout
import android.widget.TextView

class MainActivity : Activity() {
    companion object {
        init {
            System.loadLibrary("mk64_android")
        }
    }

    private external fun nativeRuntimeVersion(): Int

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.insetsController?.let {
            it.hide(WindowInsets.Type.systemBars())
            it.systemBarsBehavior = WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        val nativeVersion = nativeRuntimeVersion()
        val root = FrameLayout(this)
        val status = TextView(this).apply {
            text = "MK64 Android\nNative runtime initialized (v$nativeVersion)"
            textSize = 22f
            setPadding(32, 32, 32, 32)
        }
        root.addView(status)
        setContentView(root)
    }
}
