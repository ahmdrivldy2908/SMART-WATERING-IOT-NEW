import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'firebase_options.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );
  runApp(const MyApp());
}

// ======================================================

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      debugShowCheckedModeBanner: false,
      home: HomePage(),
    );
  }
}

// ======================================================

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  final DatabaseReference soilRef =
      FirebaseDatabase.instance.ref('soil');
  final DatabaseReference pumpStateRef =
      FirebaseDatabase.instance.ref('pump/state');
  final DatabaseReference pumpModeRef =
      FirebaseDatabase.instance.ref('pump/mode');
  final DatabaseReference pumpManualRef =
      FirebaseDatabase.instance.ref('pump/manual_state');

  bool pumpOn = false;
  bool manualPump = false;
  String pumpMode = "auto";

  int soil1 = 0;
  int soil2 = 0;
  int soil3 = 0;

  @override
  void initState() {
    super.initState();

    pumpStateRef.onValue.listen((e) {
      if (e.snapshot.value != null) {
        setState(() => pumpOn = e.snapshot.value as bool);
      }
    });

    pumpModeRef.onValue.listen((e) {
      if (e.snapshot.value != null) {
        setState(() => pumpMode = e.snapshot.value as String);
      }
    });

    pumpManualRef.onValue.listen((e) {
      if (e.snapshot.value != null) {
        setState(() => manualPump = e.snapshot.value as bool);
      }
    });

    soilRef.onValue.listen((e) {
      if (e.snapshot.value != null) {
        final d = Map<String, dynamic>.from(e.snapshot.value as Map);
        setState(() {
          soil1 = d['client1'] ?? 0;
          soil2 = d['client2'] ?? 0;
          soil3 = d['server'] ?? 0;
        });
      }
    });
  }

  // ======================================================

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFF3EDFF),
      body: SafeArea(
        child: SingleChildScrollView(
          padding: const EdgeInsets.only(bottom: 30),
          child: Column(
            children: [
              const SizedBox(height: 20),

              const Text(
                "Smart Automatic\nWatering",
                textAlign: TextAlign.center,
                style: TextStyle(
                  fontSize: 30,
                  fontWeight: FontWeight.bold,
                  color: Color(0xFF8E44AD),
                ),
              ),

              const SizedBox(height: 20),

              soilCard("Soil 1", soil1),
              soilCard("Soil 2", soil2),
              soilCard("Soil 3", soil3),

              const SizedBox(height: 25),

              bottomPanel(),
            ],
          ),
        ),
      ),
    );
  }

  // ======================================================
  // ===================== SOIL CARD ======================
  // ======================================================

  Widget soilCard(String title, int value) {
    String status;
    List<Color> gradient;

    if (value < 35) {
      status = "DRY";
      gradient = [Colors.redAccent, Colors.deepOrange];
    } else if (value < 70) {
      status = "NORMAL";
      gradient = [Colors.orangeAccent, Colors.yellowAccent];
    } else {
      status = "WET";
      gradient = [Colors.lightGreenAccent, Colors.green];
    }

    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
      child: Container(
        padding: const EdgeInsets.all(18),
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(25),
          boxShadow: [
            BoxShadow(
              color: gradient.last.withOpacity(0.4),
              blurRadius: 25,
              offset: const Offset(0, 10),
            )
          ],
        ),
        child: Row(
          children: [
            Container(
              width: 65,
              height: 65,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                gradient: RadialGradient(
                  colors: [Colors.white, gradient.last],
                ),
                boxShadow: [
                  BoxShadow(
                    color: gradient.last.withOpacity(0.6),
                    blurRadius: 18,
                  )
                ],
              ),
              child: const Center(
                child: Text("🌱", style: TextStyle(fontSize: 30)),
              ),
            ),

            const SizedBox(width: 15),

            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(title,
                      style: const TextStyle(
                          fontSize: 18, fontWeight: FontWeight.bold)),

                  const SizedBox(height: 6),

                  ClipRRect(
                    borderRadius: BorderRadius.circular(10),
                    child: LinearProgressIndicator(
                      value: value / 100,
                      minHeight: 12,
                      backgroundColor: Colors.grey.shade200,
                      valueColor:
                          AlwaysStoppedAnimation(gradient.last),
                    ),
                  ),

                  const SizedBox(height: 6),

                  Text(
                    "Status: $status",
                    style: TextStyle(
                      color: gradient.last,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                ],
              ),
            ),

            const SizedBox(width: 10),

            Text(
              "$value%",
              style: TextStyle(
                fontSize: 22,
                fontWeight: FontWeight.bold,
                color: gradient.last,
              ),
            )
          ],
        ),
      ),
    );
  }

  // ======================================================
  // ===================== BOTTOM PANEL ===================
  // ======================================================

  Widget bottomPanel() {
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 16),
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        gradient: const LinearGradient(
          colors: [Color(0xFF8E44AD), Color(0xFF5E2B97)],
        ),
        borderRadius: BorderRadius.circular(35),
        boxShadow: [
          BoxShadow(
            color: Colors.deepPurple.withOpacity(0.6),
            blurRadius: 30,
          )
        ],
      ),
      child: Row(
        children: [
          // ===== INDICATOR =====
          Expanded(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const Text(
                  "Indicator Pump",
                  style: TextStyle(color: Colors.white70),
                ),

                const SizedBox(height: 12),

                glowingCircle(
                  pumpOn,
                  text: pumpOn ? "ON" : "OFF",
                ),

                const SizedBox(height: 10),

                Text(
                  pumpOn
                      ? (pumpMode == "auto"
                          ? "Pump is running automatically"
                          : "Pump is running manually")
                      : "Pump is stopped",
                  textAlign: TextAlign.center,
                  style: const TextStyle(
                    color: Colors.white70,
                    fontSize: 12,
                  ),
                ),
              ],
            ),
          ),

          Container(
            width: 2,
            height: 150,
            color: Colors.white24,
          ),

          // ===== CONTROL =====
          Expanded(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const Text(
                  "Pump Mode",
                  style: TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.bold,
                  ),
                ),

                const SizedBox(height: 10),

                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    GestureDetector(
                      onTap: () => pumpModeRef.set("auto"),
                      child: modeChip("AUTO", pumpMode == "auto", Icons.autorenew),
                    ),
                    const SizedBox(width: 8),
                    GestureDetector(
                      onTap: () => pumpModeRef.set("manual"),
                      child: modeChip("MANUAL", pumpMode == "manual", Icons.handyman),
                    ),
                  ],
                ),

                const SizedBox(height: 18),

                GestureDetector(
                  onTap: pumpMode == "manual"
                      ? () => pumpManualRef.set(!manualPump)
                      : null,
                  child: glowingCircle(
                    manualPump,
                    text: manualPump ? "ON" : "OFF",
                  ),
                )
              ],
            ),
          )
        ],
      ),
    );
  }

  // ======================================================

  Widget modeChip(String text, bool active, IconData icon) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
      decoration: BoxDecoration(
        color: active ? Colors.greenAccent : Colors.white24,
        borderRadius: BorderRadius.circular(22),
        boxShadow: active
            ? [
                BoxShadow(
                  color: Colors.greenAccent.withOpacity(0.6),
                  blurRadius: 12,
                )
              ]
            : [],
      ),
      child: Row(
        children: [
          Icon(icon,
              size: 16,
              color: active ? Colors.black : Colors.white),
          const SizedBox(width: 4),
          Text(
            text,
            style: TextStyle(
              color: active ? Colors.black : Colors.white,
              fontWeight: FontWeight.bold,
            ),
          ),
        ],
      ),
    );
  }

  Widget glowingCircle(bool on, {required String text}) {
    return Container(
      width: 85,
      height: 85,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        gradient: RadialGradient(
          colors: on
              ? [Colors.white, Colors.greenAccent]
              : [Colors.white, Colors.grey],
        ),
        boxShadow: [
          BoxShadow(
            color: on ? Colors.greenAccent : Colors.black26,
            blurRadius: 30,
            spreadRadius: 6,
          )
        ],
      ),
      child: Center(
        child: Text(
          text,
          style: const TextStyle(
            fontSize: 20,
            fontWeight: FontWeight.bold,
          ),
        ),
      ),
    );
  }
}
