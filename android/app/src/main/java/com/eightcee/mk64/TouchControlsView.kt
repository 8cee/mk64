package com.eightcee.mk64

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.view.MotionEvent
import android.view.View
import kotlin.math.hypot

class TouchControlsView(
    context: Context,
    private val onState: (buttons: Int, stickX: Int, stickY: Int) -> Unit
) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private var buttons = 0
    private var stickX = 0
    private var stickY = 0
    private var stickPointer = -1

    init { setWillNotDraw(false) }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        paint.style = Paint.Style.FILL
        paint.alpha = 105
        val r = height * 0.105f
        val cy = height * 0.73f
        paint.color = 0xff202020.toInt()
        canvas.drawCircle(width * 0.16f, cy, r * 1.45f, paint)
        canvas.drawCircle(width * 0.84f, cy, r, paint)
        canvas.drawCircle(width * 0.72f, cy + r * 0.55f, r * 0.82f, paint)
        canvas.drawRoundRect(RectF(width * .46f, height * .82f, width * .56f, height * .90f), 24f, 24f, paint)

        paint.alpha = 220
        paint.textAlign = Paint.Align.CENTER
        paint.textSize = r * .75f
        paint.color = 0xffffffff.toInt()
        canvas.drawText("A", width * .84f, cy + r * .25f, paint)
        canvas.drawText("B", width * .72f, cy + r * .78f, paint)
        paint.textSize = r * .35f
        canvas.drawText("START", width * .51f, height * .875f, paint)
    }

    override fun onTouchEvent(e: MotionEvent): Boolean {
        val r = height * 0.105f
        val stickCx = width * .16f
        val stickCy = height * .73f
        var nextButtons = 0

        for (i in 0 until e.pointerCount) {
            val x = e.getX(i)
            val y = e.getY(i)
            val id = e.getPointerId(i)
            if (hypot(x - width * .84f, y - stickCy) <= r * 1.15f) nextButtons = nextButtons or 0x8000
            if (hypot(x - width * .72f, y - (stickCy + r * .55f)) <= r) nextButtons = nextButtons or 0x4000
            if (x in width * .46f..width * .56f && y in height * .80f..height * .92f) nextButtons = nextButtons or 0x1000

            if (id == stickPointer || (stickPointer == -1 && hypot(x - stickCx, y - stickCy) <= r * 1.8f)) {
                stickPointer = id
                stickX = (((x - stickCx) / (r * 1.45f)).coerceIn(-1f, 1f) * 80).toInt()
                stickY = ((-(y - stickCy) / (r * 1.45f)).coerceIn(-1f, 1f) * 80).toInt()
            }
        }

        if (e.actionMasked == MotionEvent.ACTION_UP || e.actionMasked == MotionEvent.ACTION_CANCEL ||
            (e.actionMasked == MotionEvent.ACTION_POINTER_UP && e.getPointerId(e.actionIndex) == stickPointer)) {
            stickPointer = -1
            stickX = 0
            stickY = 0
        }
        buttons = nextButtons
        onState(buttons, stickX, stickY)
        invalidate()
        return true
    }
}
