package com.example.iot

import android.content.ContentValues.TAG
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import com.ramcosta.composedestinations.navigation.DestinationsNavigator
import com.example.iot.ui.theme.IOTTheme
import com.ramcosta.composedestinations.annotation.Destination
import com.example.iot.NavGraphs
import com.example.iot.destinations.ViewADestination
import com.google.firebase.Firebase
import com.google.firebase.firestore.firestore
import com.ramcosta.composedestinations.DestinationsNavHost
import kotlinx.coroutines.delay

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            IOTTheme {
                Scaffold(modifier= Modifier.fillMaxSize()){
                        innerPadding->
                    DestinationsNavHost(
                        navGraph= NavGraphs.root,
                        modifier=Modifier.padding(innerPadding)
                    )
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        Log.i("MainActivity Tag", "MainActivity -> OnResume")
    }
}


@Destination(start = true)
@Composable
fun SplashScreen(navigator: DestinationsNavigator) {
    // This will control how long the splash screen stays on
    var isSplashVisible by remember { mutableStateOf(true) }

    // Automatically hide the splash screen after 3 seconds
    LaunchedEffect(Unit) {
        delay(3000)
        isSplashVisible = false
        // Navigate to your main screen using the generated destination
        navigator.navigate(ViewADestination)
    }

    if (isSplashVisible) {
        Box(
            contentAlignment = Alignment.Center,
            modifier = Modifier.fillMaxSize()
        ) {
            Image(
                painter = painterResource(id = R.drawable.ic_launcher_foreground),
                contentDescription = "App Logo",
                modifier = Modifier.size(128.dp)
            )
        }
    }
}

@Destination
@Composable
fun ViewA(
    navigator: DestinationsNavigator
) {
    // Your main content goes here
    val db = Firebase.firestore
    // Create a new user with a first and last name
    val Plante = hashMapOf(
        "ArrosageAuto" to false,
        "Nom" to "BellePlante",
        "RefHumidite" to 66
    )

    // Add a new document with a generated ID
    db.collection("Plante")
        .add(Plante)
        .addOnSuccessListener { documentReference ->
            Log.d(TAG, "DocumentSnapshot added with ID: ${documentReference.id}")
        }
        .addOnFailureListener { e ->
            Log.w(TAG, "Error adding document", e)
        }


    db.collection("Plante")
    .get()
    .addOnSuccessListener { result ->
        for (document in result) {
            Log.d(TAG, "${document.id} => ${document.data}")
        }
    }
    .addOnFailureListener { exception ->
        Log.w(TAG, "Error getting documents.", exception)
    }


}
