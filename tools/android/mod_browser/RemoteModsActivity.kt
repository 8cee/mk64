package com.izzy.kart

import android.app.AlertDialog
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

class RemoteModsActivity : ComponentActivity() {
    private lateinit var list: LinearLayout
    private lateinit var status: TextView
    private lateinit var progress: ProgressBar
    private var remoteMods: List<RemoteModRepository.RemoteMod> = emptyList()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        buildUi()
        refresh()
    }

    private fun buildUi() {
        val pad = dp(16)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad, pad, pad, pad)
            setBackgroundColor(BACKGROUND)
        }

        root.addView(TextView(this).apply {
            text = "Online Mods"
            setTextColor(Color.WHITE)
            textSize = 22f
            setTypeface(typeface, Typeface.BOLD)
        })

        root.addView(TextView(this).apply {
            text = "Community mods from the MK64 registry. Downloads install directly into the game's mods folder."
            setTextColor(SUBTLE)
            textSize = 12f
            setPadding(0, dp(4), 0, dp(8))
        })

        status = TextView(this).apply {
            setTextColor(SUBTLE)
            textSize = 13f
            gravity = Gravity.CENTER
            setPadding(0, dp(10), 0, dp(10))
        }
        root.addView(status)

        progress = ProgressBar(this).apply { visibility = View.GONE }
        root.addView(progress)

        list = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        root.addView(
            ScrollView(this).apply { addView(list) },
            LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f)
        )

        val actions = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        actions.addView(button("Refresh") { refresh() })
        actions.addView(button("Installed mods") {
            startActivity(Intent(this@RemoteModsActivity, ModsActivity::class.java))
            finish()
        })
        actions.addView(View(this), LinearLayout.LayoutParams(0, 1, 1f))
        actions.addView(button("Done") { finish() })
        root.addView(actions)

        setContentView(root)
    }

    private fun refresh() {
        setBusy(true, "Loading mod registry…")
        thread(name = "mod-registry") {
            val result = RemoteModRepository.fetch()
            runOnUiThread {
                result.fold(
                    onSuccess = {
                        remoteMods = it
                        setBusy(false, if (it.isEmpty()) "No mods are published yet." else "${it.size} mods available")
                        render()
                    },
                    onFailure = {
                        setBusy(false, "Could not load mod server: ${it.message ?: it}")
                    }
                )
            }
        }
    }

    private fun render() {
        list.removeAllViews()
        val installed = ModStore.list(this).map { it.name.lowercase() }.toSet()
        remoteMods.forEachIndexed { index, mod ->
            val row = LinearLayout(this).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
                setPadding(dp(8), dp(8), dp(8), dp(8))
                setBackgroundColor(if (index % 2 == 0) ROW_EVEN else Color.TRANSPARENT)
            }

            val textColumn = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
            textColumn.addView(TextView(this).apply {
                text = if (mod.version.isBlank()) mod.name else "${mod.name}  ${mod.version}"
                setTextColor(Color.WHITE)
                textSize = 15f
                setTypeface(typeface, Typeface.BOLD)
            })
            textColumn.addView(TextView(this).apply {
                text = buildString {
                    if (mod.author.isNotBlank()) append("by ${mod.author}")
                    if (mod.description.isNotBlank()) {
                        if (isNotEmpty()) append("\\n")
                        append(mod.description)
                    }
                }
                setTextColor(SUBTLE)
                textSize = 11f
                maxLines = 3
            })
            row.addView(textColumn, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))

            val alreadyInstalled = installed.contains(mod.fileName.lowercase())
            row.addView(button(if (alreadyInstalled) "Reinstall" else "Install") { confirmInstall(mod) })
            if (mod.homepageUrl.startsWith("https://")) {
                row.addView(button("Info") {
                    startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(mod.homepageUrl)))
                })
            }
            list.addView(row)
        }
    }

    private fun confirmInstall(mod: RemoteModRepository.RemoteMod) {
        AlertDialog.Builder(this)
            .setTitle("Install ${mod.name}?")
            .setMessage("This downloads ${mod.fileName} from the community mod server and adds it to MK64.")
            .setPositiveButton("Install") { _, _ -> install(mod) }
            .setNegativeButton("Cancel", null)
            .show()
    }

    private fun install(mod: RemoteModRepository.RemoteMod) {
        setBusy(true, "Downloading ${mod.name}…")
        thread(name = "mod-download") {
            val error = RemoteModRepository.install(this, mod) { downloaded, total ->
                if (total > 0) {
                    val pct = (downloaded * 100L / total).coerceIn(0L, 100L)
                    runOnUiThread { status.text = "Downloading ${mod.name}… $pct%" }
                }
            }
            runOnUiThread {
                if (error == null) {
                    Toast.makeText(
                        this,
                        "${mod.name} installed. Restart the game to apply it.",
                        Toast.LENGTH_LONG
                    ).show()
                    setBusy(false, "Installed ${mod.name}")
                    render()
                } else {
                    setBusy(false, error)
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

    private companion object {
        val BACKGROUND = Color.rgb(8, 16, 28)
        val SUBTLE = Color.rgb(150, 160, 175)
        val ROW_EVEN = Color.argb(24, 255, 255, 255)
    }
}
