package ui;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSlider;
import javax.swing.Timer;
import java.awt.BorderLayout;
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

public class ControlPanel extends JPanel {

    private final JButton btnReset;
    private final JButton btnStepBack;
    private final JButton btnPlayPause;
    private final JButton btnStepForward;
    private final JButton btnEnd;

    private final JSlider speedSlider;
    private final Timer simulationTimer;
    private boolean isPlaying = false;

    private final JLabel stepCounterLabel;
    private final JLabel deliveredValueLabel;
    private final JLabel energyValueLabel;

    private final JLabel commandStatusLabel;

    public interface ControlEventListener {
        void onStepForward();
        void onStepBackward();
        void onReset();
        void onEnd();
        void onPlayStateChanged(boolean playing);
        void onSpeedChanged(int delayMs);
    }

    private ControlEventListener controlListener;

    public ControlPanel() {
        setLayout(new BorderLayout(15, 0));
        setBackground(UITheme.PANEL_BG);
        setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createMatteBorder(1, 0, 0, 0, UITheme.PANEL_BORDER),
            BorderFactory.createEmptyBorder(12, 20, 14, 20)
        ));

        JPanel leftPanel = new JPanel();
        leftPanel.setLayout(new BoxLayout(leftPanel, BoxLayout.Y_AXIS));
        leftPanel.setOpaque(false);

        JPanel buttonRow = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 0));
        buttonRow.setOpaque(false);

        btnReset = createStyledButton("Başa");
        btnStepBack = createStyledButton("Geri");
        btnPlayPause = createStyledButton("Oynat");
        btnStepForward = createStyledButton("İleri");
        btnEnd = createStyledButton("Sona");

        buttonRow.add(btnReset);
        buttonRow.add(btnStepBack);
        buttonRow.add(btnPlayPause);
        buttonRow.add(btnStepForward);
        buttonRow.add(btnEnd);

        JPanel sliderRow = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 4));
        sliderRow.setOpaque(false);

        JLabel speedLabel = new JLabel("Hız:");
        speedLabel.setFont(UITheme.FONT_BADGE);
        speedLabel.setForeground(UITheme.TEXT_SECONDARY);

        speedSlider = new JSlider(1, 9, 5);
        speedSlider.setInverted(false);
        speedSlider.setOpaque(false);
        speedSlider.setPreferredSize(new Dimension(130, 20));

        sliderRow.add(speedLabel);
        sliderRow.add(speedSlider);

        leftPanel.add(buttonRow);
        leftPanel.add(Box.createVerticalStrut(4));
        leftPanel.add(sliderRow);

        add(leftPanel, BorderLayout.WEST);

        JPanel centerPanel = new JPanel();
        centerPanel.setLayout(new BoxLayout(centerPanel, BoxLayout.Y_AXIS));
        centerPanel.setOpaque(false);

        stepCounterLabel = new JLabel("Kare 0 / 17 (Komut 0 / 4)");
        stepCounterLabel.setFont(new java.awt.Font("Segoe UI", java.awt.Font.PLAIN, 11));
        stepCounterLabel.setForeground(UITheme.TEXT_MUTED);
        stepCounterLabel.setAlignmentX(0.5f);

        JPanel metricsRow = new JPanel(new FlowLayout(FlowLayout.CENTER, 25, 0));
        metricsRow.setOpaque(false);

        JPanel deliveredCard = new JPanel();
        deliveredCard.setLayout(new BoxLayout(deliveredCard, BoxLayout.Y_AXIS));
        deliveredCard.setOpaque(false);

        deliveredValueLabel = new JLabel("0 / 4");
        deliveredValueLabel.setFont(new java.awt.Font("Segoe UI", java.awt.Font.BOLD, 22));
        deliveredValueLabel.setForeground(UITheme.TEXT_PRIMARY);
        deliveredValueLabel.setAlignmentX(0.5f);

        JLabel deliveredTitle = new JLabel("teslim edilen paket");
        deliveredTitle.setFont(new java.awt.Font("Segoe UI", java.awt.Font.PLAIN, 11));
        deliveredTitle.setForeground(UITheme.TEXT_SECONDARY);
        deliveredTitle.setAlignmentX(0.5f);

        deliveredCard.add(deliveredValueLabel);
        deliveredCard.add(deliveredTitle);

        JPanel energyCard = new JPanel();
        energyCard.setLayout(new BoxLayout(energyCard, BoxLayout.Y_AXIS));
        energyCard.setOpaque(false);

        energyValueLabel = new JLabel("80");
        energyValueLabel.setFont(new java.awt.Font("Segoe UI", java.awt.Font.BOLD, 22));
        energyValueLabel.setForeground(UITheme.GREEN_PRIMARY);
        energyValueLabel.setAlignmentX(0.5f);

        JLabel energyTitle = new JLabel("kalan enerji toplamı");
        energyTitle.setFont(new java.awt.Font("Segoe UI", java.awt.Font.PLAIN, 11));
        energyTitle.setForeground(UITheme.TEXT_SECONDARY);
        energyTitle.setAlignmentX(0.5f);

        energyCard.add(energyValueLabel);
        energyCard.add(energyTitle);

        metricsRow.add(deliveredCard);
        metricsRow.add(energyCard);

        centerPanel.add(stepCounterLabel);
        centerPanel.add(Box.createVerticalStrut(4));
        centerPanel.add(metricsRow);

        add(centerPanel, BorderLayout.CENTER);

        JPanel rightPanel = new JPanel(new BorderLayout());
        rightPanel.setBackground(UITheme.BG_SPACE);
        rightPanel.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(UITheme.PANEL_BORDER, 1),
            BorderFactory.createEmptyBorder(10, 14, 10, 14)
        ));
        rightPanel.setPreferredSize(new Dimension(380, 0));

        JLabel cmdTitle = new JLabel("Aktif Komut Durumu");
        cmdTitle.setFont(UITheme.FONT_BADGE);
        cmdTitle.setForeground(UITheme.GREEN_PRIMARY);

        commandStatusLabel = new JLabel("Komutlar yüklenmeye hazır. 'İleri' veya 'Oynat' ile başlatın.");
        commandStatusLabel.setFont(UITheme.FONT_REGULAR);
        commandStatusLabel.setForeground(UITheme.TEXT_PRIMARY);

        rightPanel.add(cmdTitle, BorderLayout.NORTH);
        rightPanel.add(Box.createVerticalStrut(4), BorderLayout.CENTER);
        rightPanel.add(commandStatusLabel, BorderLayout.SOUTH);

        add(rightPanel, BorderLayout.EAST);

        simulationTimer = new Timer(getDelayForSpeed(speedSlider.getValue()), e -> {
            if (controlListener != null) {
                controlListener.onStepForward();
            }
        });

        setupActions();
    }

    public static int getDelayForSpeed(int level) {
        switch (level) {
            case 1: return 1200;
            case 2: return 1000;
            case 3: return 800;
            case 4: return 600;
            case 5: return 450;
            case 6: return 300;
            case 7: return 180;
            case 8: return 100;
            case 9: return 50;
            default: return 450;
        }
    }

    private void setupActions() {
        btnPlayPause.addActionListener(e -> togglePlayPause());

        btnStepForward.addActionListener(e -> {
            if (isPlaying) togglePlayPause();
            if (controlListener != null) controlListener.onStepForward();
        });

        btnStepBack.addActionListener(e -> {
            if (isPlaying) togglePlayPause();
            if (controlListener != null) controlListener.onStepBackward();
        });

        btnReset.addActionListener(e -> {
            if (isPlaying) togglePlayPause();
            if (controlListener != null) controlListener.onReset();
        });

        btnEnd.addActionListener(e -> {
            if (isPlaying) togglePlayPause();
            if (controlListener != null) controlListener.onEnd();
        });

        speedSlider.addChangeListener(e -> {
            int delay = getDelayForSpeed(speedSlider.getValue());
            simulationTimer.setDelay(delay);
            if (controlListener != null) controlListener.onSpeedChanged(delay);
        });
    }

    public void togglePlayPause() {
        isPlaying = !isPlaying;
        if (isPlaying) {
            btnPlayPause.setText("Durdur");
            btnPlayPause.setBackground(new java.awt.Color(220, 38, 38));
            simulationTimer.start();
        } else {
            btnPlayPause.setText("Oynat");
            btnPlayPause.setBackground(UITheme.GREEN_DARK);
            simulationTimer.stop();
        }
        if (controlListener != null) {
            controlListener.onPlayStateChanged(isPlaying);
        }
    }

    public void stopSimulation() {
        if (isPlaying) {
            togglePlayPause();
        }
    }

    public void updateMetrics(int currentStep, int totalSteps, int currentCmd, int totalCmd, int delivered, int totalRobots, int remainingEnergy) {
        stepCounterLabel.setText("Kare " + currentStep + " / " + totalSteps + " (Komut " + currentCmd + " / " + totalCmd + ")");
        deliveredValueLabel.setText(delivered + " / " + totalRobots);
        energyValueLabel.setText(String.valueOf(remainingEnergy));
    }

    public void setStatusMessage(String message) {
        commandStatusLabel.setText(message);
    }

    private JButton createStyledButton(String text) {
        JButton button = new JButton(text);
        button.setFont(UITheme.FONT_BUTTON);
        button.setForeground(UITheme.TEXT_PRIMARY);
        button.setBackground(UITheme.GREEN_DARK);
        button.setFocusPainted(false);
        button.setCursor(new Cursor(Cursor.HAND_CURSOR));
        button.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(UITheme.GREEN_BORDER, 1),
            BorderFactory.createEmptyBorder(6, 12, 6, 12)
        ));

        button.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseEntered(MouseEvent e) {
                if (!button.getText().contains("Durdur")) {
                    button.setBackground(UITheme.GREEN_HOVER);
                }
            }

            @Override
            public void mouseExited(MouseEvent e) {
                if (!button.getText().contains("Durdur")) {
                    button.setBackground(UITheme.GREEN_DARK);
                }
            }
        });

        return button;
    }

    public void setControlListener(ControlEventListener listener) {
        this.controlListener = listener;
    }
}
