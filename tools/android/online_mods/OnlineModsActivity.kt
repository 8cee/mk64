package com.izzy.kart

import android.content.Intent
import android.graphics.Color
import android.graphics.Typeface
import android.net.Uri
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import androidx.activity.ComponentActivity
import kotlin.concurrent.thread

class OnlineModsActivity : ComponentActivity() {
    private lateinit var list: LinearLayout
    private lateinit var status: TextView
    private lateinit var progress: ProgressBar

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        buildUi()
        refreshCatalog()
    }

    private fun buildUi() {
        val pad = dp(16)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad, pad, pad, pad)
            setBackgroundColor(Color.rgb(8, 16, 28))
        }

        root.addView(TextView(this).apply {
            text = "Online Mods"
            setTextColor(Color.WHITE)
            textSize = 22f
            setTypeface(typeface, Typeface.BOLD)
        })

        root.addView(TextView(this).apply {
            text = "Community mods for SpaghettiKart. Downloads are resolved from GameBanana when you install them."
            setTextColor(Color.rgb(150, 160, 175))
            textSize = 12f
            setPadding(0, dp(4), 0, dp(8))
        })

        val bar = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        bar.addView(button("Refresh") { refreshCatalog() })
        bar.addView(button("Installed mods") {
            startActivity(Intent(this@OnlineModsActivity, ModsActivity::class.java))
            finish()
        })
        bar.addView(View(this), LinearLayout.LayoutParams(0, 1, 1f))
        bar.addView(button("Done") { finish() })
        root.addView(bar)

        progress = ProgressBar(this).apply { visibility = View.GONE }
        root.addView(progress)

        status = TextView(this).apply {
            setTextColor(Color.rgb(180, 190, 205))
            gravity = Gravity.CENTER
            setPadding(0, dp(12), 0, dp(12))
        }
        root.addView(status)

        list = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        root.addView(
            ScrollView(this).apply { addView(list) },
            LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f)
        )

        setContentView(root)
    }

    private fun refreshCatalog() {
        setBusy(true, "Loading mod catalog…")
        list.removeAllViews()

        thread(name = "mod-catalog") {
            val result = runCatching { OnlineModCatalog.fetch() }
            runOnUiThread {
                result.onSuccess { entries ->
                    setBusy(false, if (entries.isEmpty()) "No mods are published yet." else "")
                    entries.forEachIndexed { index, entry -> list.addView(row(entry, index)) }
                }.onFailure {
                    setBusy(false, "Could not load mod catalog: \${it.message ?: it}")
                }
            }
        }
    }

    private fun row(entry: OnlineModCatalog.Entry, index: Int): View {
        val row = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(10), dp(10), dp(10), dp(10))
            setBackgroundColor(if (index % 2 == 0) Color.argb(24, 255, 255, 255) else Color.TRANSPARENT)
        }

        row.addView(TextView(this).apply {
            text = entry.name
            setTextColor(Color.WHITE)
            textSize = 16f
            setTypeface(typeface, Typeface.BOLD)
        })

        row.addView(TextView(this).apply {
            text = entry.category
            setTextColor(Color.rgb(110, 175, 235))
            textSize = 11f
        })

        row.addView(TextView(this).apply {
            text = entry.description
            setTextColor(Color.rgb(185, 192, 205))
            textSize = 12f
            setPadding(0, dp(4), 0, dp(6))
        })

        val actions = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        actions.addView(button("Download & install") { install(entry) })
        actions.addView(button("GameBanana") {
            startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(entry.homepage)))
        })
        row.addView(actions)
        return row
    }

    private fun install(entry: OnlineModCatalog.Entry) {
        setBusy(true, "Downloading \${entry.name}…")
        thread(name = "mod-download") {
            val result = runCatching { OnlineModCatalog.install(this, entry) }
            runOnUiThread {
                result.onSuccess { file ->
                    setBusy(false, "")
                    Toast.makeText(
                        this,
                        "\${entry.name} installed as $file. Restart the game to load it.",
                        Toast.LENGTH_LONG
                    ).show()
                }.onFailure {
                    setBusy(false, "Install failed: \${it.message ?: it}")
                }
            }
        }
    }

    private fun setBusy(busy: Boolean, message: String) {
        progress.visibility = if (busy) View.VISIBLE else View.GONE
        status.text = message
    }

    private fun button(label: String, click: () -> Unit) = Button(this).apply {
        text = label
        textSize = 12f
        isAllCaps = false
        minWidth = 0
        minimumWidth = 0
        setPadding(dp(10), dp(4), dp(10), dp(4))
        setOnClickListener { click() }
    }

    private fun dp(value: Int) = (value * resources.displayMetrics.density).toInt()
}
