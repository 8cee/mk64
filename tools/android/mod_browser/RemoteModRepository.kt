package com.izzy.kart

import android.content.Context
import android.util.Log
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.net.HttpURLConnection
import java.net.URL
import java.security.MessageDigest

object RemoteModRepository {
    private const val TAG = "RemoteMods"
    private const val REGISTRY_URL =
        "https://raw.githubusercontent.com/8cee/mk64/android-spaghettikart-runtime/mods/registry.json"
    private const val USER_AGENT = "MK64-Android-ModBrowser/1.0"
    private const val MAX_DOWNLOAD_BYTES = 512L * 1024L * 1024L

    data class RemoteMod(
        val id: String,
        val name: String,
        val version: String,
        val author: String,
        val description: String,
        val downloadUrl: String,
        val homepageUrl: String,
        val fileName: String,
        val sha256: String
    )

    fun fetch(): Result<List<RemoteMod>> = runCatching {
        val text = getText(REGISTRY_URL)
        val root = JSONObject(text)
        val schema = root.optInt("schema", 0)
        require(schema == 1) { "Unsupported mod registry schema: $schema" }

        val mods = root.getJSONArray("mods")
        buildList {
            for (index in 0 until mods.length()) {
                val item = mods.getJSONObject(index)
                val url = item.getString("download_url")
                require(url.startsWith("https://")) { "Unsafe download URL for ${item.optString("name")}" }

                val fileName = item.optString("file_name")
                    .ifBlank { url.substringBefore('?').substringAfterLast('/') }
                require(fileName.endsWith(".zip", true) || fileName.endsWith(".o2r", true)) {
                    "Unsupported mod archive: $fileName"
                }

                add(
                    RemoteMod(
                        id = item.getString("id"),
                        name = item.getString("name"),
                        version = item.optString("version"),
                        author = item.optString("author"),
                        description = item.optString("description"),
                        downloadUrl = url,
                        homepageUrl = item.optString("homepage_url"),
                        fileName = fileName,
                        sha256 = item.optString("sha256").lowercase()
                    )
                )
            }
        }
    }

    fun install(context: Context, mod: RemoteMod, progress: (Long, Long) -> Unit = { _, _ -> }): String? {
        val safeName = mod.fileName
            .substringAfterLast('/')
            .replace(Regex("""[^A-Za-z0-9._()\\- ]"""), "_")

        if (!safeName.endsWith(".zip", true) && !safeName.endsWith(".o2r", true)) {
            return "Unsupported mod archive: $safeName"
        }

        val temp = File(context.cacheDir, "mod-download-${System.nanoTime()}-$safeName")
        return try {
            download(mod.downloadUrl, temp, progress)
            if (mod.sha256.isNotBlank()) {
                val actual = sha256(temp)
                if (!actual.equals(mod.sha256, ignoreCase = true)) {
                    return "Checksum mismatch for ${mod.name}. Download discarded."
                }
            }
            ModStore.importDownloadedFile(context, temp, safeName)
        } catch (error: Exception) {
            Log.e(TAG, "Download failed for ${mod.name}", error)
            "Could not download ${mod.name}: ${error.message ?: error}"
        } finally {
            temp.delete()
        }
    }

    private fun getText(url: String): String {
        val connection = open(url)
        return connection.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() }
            .also { connection.disconnect() }
    }

    private fun download(url: String, target: File, progress: (Long, Long) -> Unit) {
        val connection = open(url)
        val expected = connection.contentLengthLong
        if (expected > MAX_DOWNLOAD_BYTES) {
            connection.disconnect()
            error("Mod is too large (${expected / (1024 * 1024)} MB).")
        }

        var total = 0L
        connection.inputStream.use { input ->
            FileOutputStream(target).use { output ->
                val buffer = ByteArray(128 * 1024)
                while (true) {
                    val read = input.read(buffer)
                    if (read < 0) break
                    total += read
                    if (total > MAX_DOWNLOAD_BYTES) error("Mod exceeds the 512 MB download limit.")
                    output.write(buffer, 0, read)
                    progress(total, expected)
                }
                output.fd.sync()
            }
        }
        connection.disconnect()
    }

    private fun open(url: String): HttpURLConnection {
        require(url.startsWith("https://")) { "Only HTTPS mod sources are allowed." }
        return (URL(url).openConnection() as HttpURLConnection).apply {
            instanceFollowRedirects = true
            connectTimeout = 15_000
            readTimeout = 60_000
            setRequestProperty("User-Agent", USER_AGENT)
            connect()
            if (responseCode !in 200..299) {
                val code = responseCode
                disconnect()
                error("Server returned HTTP $code")
            }
        }
    }

    private fun sha256(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        file.inputStream().use { input ->
            val buffer = ByteArray(128 * 1024)
            while (true) {
                val read = input.read(buffer)
                if (read < 0) break
                digest.update(buffer, 0, read)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }
}
