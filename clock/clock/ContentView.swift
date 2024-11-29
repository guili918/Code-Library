//
//  ContentView.swift
//  clock
//
//  Created by Duan on 2024/11/28.
//

import SwiftUI
import AVFoundation

struct ContentView: View {
    @State private var timeElapsed: Int = 0
    @State private var timer: Timer?
    @State private var isRunning = false
    @State private var showControls = true
    @State private var showColon = true
    @State private var blinkTimer: Timer?
    @State private var showDigits = true
    @State private var playButtonScale: CGFloat = 1.0
    @State private var resetButtonScale: CGFloat = 1.0
    @State private var isButtonEnabled = true
    @State private var buttonPlayer: AVAudioPlayer?
    @State private var timerStartPlayer: AVAudioPlayer?
    
    var timeComponents: (h1: Int, h2: Int, m1: Int, m2: Int, s1: Int, s2: Int) {
        let hours = timeElapsed / 3600
        let minutes = (timeElapsed % 3600) / 60
        let seconds = timeElapsed % 60
        
        return (
            h1: hours / 10,
            h2: hours % 10,
            m1: minutes / 10,
            m2: minutes % 10,
            s1: seconds / 10,
            s2: seconds % 10
        )
    }
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                Image("background")
                    .resizable()
                    .aspectRatio(contentMode: .fill)
                    .frame(width: geometry.size.width, height: geometry.size.height)
                    .clipped()
                    .edgesIgnoringSafeArea(.all)
                
                VStack(spacing: 0) {
                    Image("title_image")
                        .resizable()
                        .aspectRatio(contentMode: .fit)
                        .frame(height: 32)
                        .padding(.top, 80)
                        .padding(.bottom, 34)
                    
                    HStack(spacing: 10) {
                        Group {
                            Image("num_\(timeComponents.h1)")
                                .resizable()
                                .aspectRatio(contentMode: .fit)
                                .frame(height: 84)
                            Image("num_\(timeComponents.h2)")
                                .resizable()
                                .aspectRatio(contentMode: .fit)
                                .frame(height: 84)
                        }
                        .opacity(!isRunning && !showDigits ? 0 : 1)
                        
                        Image("num_colon")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 84)
                            .opacity(showColon ? 1 : 0)
                        
                        Group {
                            Image("num_\(timeComponents.m1)")
                                .resizable()
                                .aspectRatio(contentMode: .fit)
                                .frame(height: 84)
                            Image("num_\(timeComponents.m2)")
                                .resizable()
                                .aspectRatio(contentMode: .fit)
                                .frame(height: 84)
                        }
                        .opacity(!isRunning && !showDigits ? 0 : 1)
                        
                        Image("num_colon")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 84)
                            .opacity(showColon ? 1 : 0)
                        
                        Group {
                            Image("num_\(timeComponents.s1)")
                                .resizable()
                                .aspectRatio(contentMode: .fit)
                                .frame(height: 84)
                            Image("num_\(timeComponents.s2)")
                                .resizable()
                                .aspectRatio(contentMode: .fit)
                                .frame(height: 84)
                        }
                        .opacity(!isRunning && !showDigits ? 0 : 1)
                    }
                    .frame(maxHeight: .infinity, alignment: .center)
                    .padding(.bottom, 44)
                    
                    if showControls {
                        HStack(spacing: 30) {
                            Button(action: {
                                guard isButtonEnabled else { return }
                                
                                isButtonEnabled = false
                                playButtonSound()
                                
                                withAnimation(.spring(response: 0.1, dampingFraction: 1)) {
                                    playButtonScale = 0.8
                                }
                                
                                DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                                    withAnimation(.spring(response: 0.1, dampingFraction: 1)) {
                                        playButtonScale = 1.0
                                    }
                                }
                                
                                if isRunning {
                                    pauseTimer()
                                } else {
                                    startTimer()
                                }
                                
                                DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                                    isButtonEnabled = true
                                }
                            }) {
                                Image(isRunning ? "pause_button" : "play_button")
                                    .resizable()
                                    .aspectRatio(contentMode: .fit)
                                    .frame(width: 60, height: 60)
                            }
                            .scaleEffect(playButtonScale)
                            .buttonStyle(PlainButtonStyle())
                            
                            Button(action: {
                                playButtonSound()
                                
                                withAnimation(.spring(response: 0.1, dampingFraction: 1)) {
                                    resetButtonScale = 0.8
                                }
                                
                                DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                                    withAnimation(.spring(response: 0.1, dampingFraction: 1)) {
                                        resetButtonScale = 1.0
                                    }
                                }
                                
                                resetTimer()
                            }) {
                                Image("reset_button")
                                    .resizable()
                                    .aspectRatio(contentMode: .fit)
                                    .frame(width: 60, height: 60)
                            }
                            .scaleEffect(resetButtonScale)
                            .buttonStyle(PlainButtonStyle())
                        }
                        .padding(.bottom, 50)
                    } else {
                        Color.clear
                            .frame(height: 110)
                    }
                }
                .frame(maxHeight: .infinity)
                .offset(y: 0)
            }
        }
        .ignoresSafeArea()
        .onTapGesture {
            withAnimation {
                showControls.toggle()
            }
        }
        .statusBar(hidden: true)
        .preferredColorScheme(.dark)
        .onDisappear {
            pauseTimer()
            blinkTimer?.invalidate()
            blinkTimer = nil
        }
        .onAppear {
            initializeAudio()
        }
    }
    
    private func startTimer() {
        cleanupTimers()
        
        showColon = true
        showDigits = true
        isRunning = true
        
        playTimerStartSound()
        
        timer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { _ in
            timeElapsed += 1
        }
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
            blinkTimer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { _ in
                showColon.toggle()
            }
        }
    }
    
    private func pauseTimer() {
        cleanupTimers()
        
        isRunning = false
        showColon = true
        showDigits = true
        
        timerStartPlayer?.stop()
        
        DispatchQueue.main.async {
            blinkTimer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { _ in
                showColon.toggle()
                showDigits.toggle()
            }
        }
    }
    
    private func resetTimer() {
        cleanupTimers()
        isRunning = false
        timeElapsed = 0
        showColon = true
        showDigits = true
        
        timerStartPlayer?.stop()
    }
    
    private func cleanupTimers() {
        timer?.invalidate()
        timer = nil
        blinkTimer?.invalidate()
        blinkTimer = nil
    }
    
    private func initializeAudio() {
        if let buttonSoundURL = Bundle.main.url(forResource: "button_click", withExtension: "mp3") {
            do {
                buttonPlayer = try AVAudioPlayer(contentsOf: buttonSoundURL)
                buttonPlayer?.prepareToPlay()
            } catch {
                print("Error loading button sound: \(error.localizedDescription)")
            }
        }
        
        if let timerSoundURL = Bundle.main.url(forResource: "timer_start", withExtension: "mp3") {
            do {
                timerStartPlayer = try AVAudioPlayer(contentsOf: timerSoundURL)
                timerStartPlayer?.prepareToPlay()
            } catch {
                print("Error loading timer sound: \(error.localizedDescription)")
            }
        }
    }
    
    private func playButtonSound() {
        buttonPlayer?.currentTime = 0
        buttonPlayer?.play()
    }
    
    private func playTimerStartSound() {
        timerStartPlayer?.currentTime = 0
        timerStartPlayer?.play()
    }
}

#Preview {
    ContentView()
}
