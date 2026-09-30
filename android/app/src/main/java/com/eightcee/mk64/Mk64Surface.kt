package com.eightcee.mk64

import android.content.Context
import android.opengl.GLSurfaceView
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class Mk64Surface(context: Context) : GLSurfaceView(context), GLSurfaceView.Renderer {
    init {
        setEGLContextClientVersion(2)
        setRenderer(this)
        renderMode = RENDERMODE_CONTINUOUSLY
        preserveEGLContextOnPause = true
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        nativeSurfaceCreated()
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        nativeSurfaceChanged(width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        nativeRenderFrame()
    }

    private external fun nativeSurfaceCreated()
    private external fun nativeSurfaceChanged(width: Int, height: Int)
    private external fun nativeRenderFrame()

    companion object {
        init { System.loadLibrary("mk64_android") }
    }
}
