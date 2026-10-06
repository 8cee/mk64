package com.izzy.kart

import android.content.Context
import android.util.Log
import org.json.JSONObject
import java.io.BufferedInputStream
import java.io.File
import java.io.FileOutputStream
import java.net.HttpURLConnection
import java.net.URL

object OnlineModCatalog {
    private const val TAG = "OnlineModCatalog"
    private const val CATALOG_URL =
        "https://raw.githubusercontent.com/8cee/mk64/android-spaghettikart-runtime/mods/catalog.json"
    private const val MAX_DOWNLOAD_BYTES = 512L * 1024L * 1024L
    private const val BUFFER = 64 * 1024

    data class Entry(
        val id: String,
        val name: String,
        val category: String,
        val description: String,
        val gameBananaId: Long,
        val homepage: String
    )

    data class RemoteFile(val name: String, val url: String)

    fun fetch(): List<Entry> {
        val root = JSONObject(readText(CATALOG_URL))
        if (root.optInt("schema", 0) != 1) throw IllegalStateException("Unsupported mod catalog format.")
        val array = root.getJSONArray("mods")
        val result = mutableListOf<Entry>()
        for (i in 0 until array.length()) {
            val item = array.getJSONObject(i)
            val gameBananaId = item.getLong("gamebananaId")
            result += Entry(
                id = item.getString("id"),
                name = item.getString("name"),
                category = item.optString("category", "Other"),
                description = item.optString("description", ""),
                gameBananaId = gameBananaId,
                homepage = item.optString("homepage", "https://gamebanana.com/mods/$gameBananaId")
            )
        }
        return result
    }

    fun resolveFile(entry: Entry): RemoteFile {
        val api = "https://gamebanana.com/apiv11/Mod/\${entry.gameBananaId}?_csvProperties=_aFiles"
        val files = JSONObject(readText(api)).getJSONArray("_aFiles")
        val candidates = mutableListOf<RemoteFile>()
        for (i in 0 until files.length()) {
            val file = files.getJSONObject(i)
            val name = file.optString("_sFile")
            val url = file.optString("_sDownloadUrl")
            if (name.isBlank() || url.isBlank()) continue
            if (name.endsWith(".o2r", true) || name.endsWith(".zip", true)) {
                candidates += RemoteFile(name, url)
            }
        }
        return candidates.firstOrNull { it.name.endsWith(".o2r", true) }
            ?: candidates.firstOrNull()
            ?: throw IllegalStateException("This mod has no Android-compatible .o2r or .zip download.")
    }

    fun install(context: Context, entry: Entry): String {
        val remote = resolveFile(entry)
        requireHttps(remote.url)

        val safeName = remote.name
            .substringAfterLast('/')
            .replace(Regex("""[^A-Za-z0-9._()\- ]"""), "_")
        if (!safeName.endsWith(".o2r", true) && !safeName.endsWith(".zip", true)) {
            throw IllegalStateException("Unsupported mod file: $safeName")
        }

        val dir = GameAssets.modsDir(context)
        dir.mkdirs()
        val order = ModStore.list(context).size
        var target = File(dir, "%03d_%s".format(order, safeName))
        var n = 1
        while (target.exists()) {
            target = File(dir, "%03d_%d_%s".format(order, n++, safeName))
        }

        val connection = open(remote.url)
        try {
            val advertised = connection.contentLengthLong
            if (advertised > MAX_DOWNLOAD_BYTES) {
                throw IllegalStateException("Mod is too large (\${advertised / (1024 * 1024)} MB).")
            }

            var total = 0L
            BufferedInputStream(connection.inputStream, BUFFER).use { input ->
                FileOutputStream(target).use { output ->
                    val buffer = ByteArray(BUFFER)
                    while (true) {
                        val read = input.read(buffer)
                        if (read < 0) break
                        total += read
                        if (total > MAX_DOWNLOAD_BYTES) {
                            throw IllegalStateException("Mod exceeded the 512 MB download limit.")
                        }
                        output.write(buffer, 0, read)
                    }
                }
            }

            if (target.length() == 0L) throw IllegalStateException("Downloaded mod is empty.")
            return target.name
        } catch (error: Exception) {
            target.delete()
            Log.e(TAG, "Install failed for \${entry.id}", error)
            throw error
        } finally {
            connection.disconnect()
        }
    }

    private fun readText(url: String): String {
        requireHttps(url)
        val connection = open(url)
        return try {
            connection.inputStream.bufferedReader().use { it.readText() }
        } finally {
            connection.disconnect()
        }
    }

    private fun open(url: String): HttpURLConnection {
        requireHttps(url)
        val connection = URL(url).openConnection() as HttpURLConnection
        connection.connectTimeout = 15_000
        connection.readTimeout = 30_000
        connection.instanceFollowRedirects = true
        connection.setRequestProperty("User-Agent", "MK64-Android-ModBrowser/1")
        connection.connect()
        if (connection.responseCode !in 200..299) {
            val code = connection.responseCode
            connection.disconnect()
            throw IllegalStateException("Server returned HTTP $code.")
        }
        return connection
    }

    private fun requireHttps(url: String) {
        if (!url.startsWith("https://", ignoreCase = true)) {
            throw IllegalArgumentException("Only HTTPS mod downloads are allowed.")
        }
    }
}
