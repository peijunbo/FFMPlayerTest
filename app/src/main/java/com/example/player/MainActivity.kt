package com.example.player

import android.os.Bundle
import android.util.Log
import android.view.Surface
import androidx.appcompat.app.AppCompatActivity
import com.example.player.databinding.ActivityMainBinding
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        writeRawFile(R.raw.gosick, "gosick.mp4")
        writeRawFile(R.raw.test, "test.mp4")
        binding.surface1.playerId = 1
        binding.surface1.init(getFilePath("gosick.mp4"))
        binding.audioButton.setOnClickListener {
            //startAudio(getFilePath("gosick.mp4"))
        }
        binding.playButton.setOnClickListener {
            binding.surface1.play()
            // startPlayer(binding.surface.holder.surface,getFilePath("gosick.mp4"))
        }

    }

    //external fun startPlayer(surface: Surface, url: String)
    //external fun startAudio(url: String)

    fun writeRawFile(resId: Int, name: String) {

        val file = File(filesDir.absolutePath + File.separator + name)
        if (file.exists()) return
        else {
            file.createNewFile()
        }
        val inputStream = resources.openRawResource(resId)
        val outputStream = FileOutputStream(file)
        outputStream.write(inputStream.readBytes())
    }

    fun getFilePath(name: String): String {
        return filesDir.canonicalPath + File.separator + name
    }

    override fun onResume() {
        super.onResume()
        binding.surface1.onResume()
        binding.surface2.onResume()
    }

    companion object {
        private const val TAG = "MainActivity"
        // Used to load the 'player' library on application startup.
        init {
            System.loadLibrary("ffmpeg-player")
        }
    }
}