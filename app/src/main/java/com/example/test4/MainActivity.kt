package com.example.test4

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.TextView
import com.example.test4.databinding.ActivityMainBinding

import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.DragInteraction
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.input.TextFieldState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.isTraversalGroup
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.traversalIndex
import androidx.compose.ui.unit.dp
import com.example.test4.ui.theme.MyApplicationTheme


interface Callback {
    fun onResult(result: String)
}
var dummy : MutableState<Int> = mutableStateOf(0)


class MainActivity : ComponentActivity() {
    external fun capitalize(input: String): Array<String>

    external fun startThread(callback: Callback)

    external fun SIGNALS()

    companion object {
        init { System.loadLibrary("test4") }
        lateinit var instance: MainActivity
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        instance = this

        startThread(object : Callback {
            override fun onResult(result: String) {
                // Ensure Toast is on UI thread
                dummy.value += 1;
            }
        })
        setContent { MyApplicationTheme { InputDisplayApp() } }
    }

}

@Composable
fun InputDisplayApp() {


    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.Top
    ) {

        var searchState = remember { mutableStateOf("") }
        var extand = remember { mutableStateOf(false) }

        MySimpleSearch(searchState, extand,)
    }
}

@Composable
fun MySimpleSearch(
    searchState: MutableState<String>,
    extand: MutableState<Boolean>,
    modifier: Modifier = Modifier
) {
    Column (
        modifier
            .fillMaxSize()
            .semantics { isTraversalGroup = true }
    ) {
        TextField(
            value = searchState.value,
            onValueChange = {
                searchState.value = it
                extand.value = true

            },
            label = { Text("heh")},
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(Modifier.size(20.dp))
        if (extand.value){
            Column(Modifier.fillMaxHeight().verticalScroll(rememberScrollState()).padding(10.dp)) {
                val x = dummy.value
                MainActivity.instance.capitalize(searchState.value).forEach { option ->
                    ListItem(
                        headlineContent = { Text(option) },
                        modifier = Modifier
                            .clickable {
                                MainActivity.instance.SIGNALS()
                                searchState.value = option
                                extand.value = false
                                Log.i("TAG", "hejjjjjjjjj Informational message")

                            }
                            .fillMaxWidth()
                    )
                }
            }
        }

    }
}