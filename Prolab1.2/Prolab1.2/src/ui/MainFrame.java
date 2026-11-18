package ui;

import command.Command;
import engine.Grid;
import engine.SimulationManager;
import io.CommandFileReader;
import io.ConsoleOutputFormatter;
import io.InvalidCommandException;
import io.InvalidMapException;
import io.MapFileReader;
import model.robot.Robot;

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class MainFrame extends JFrame {

    private final HeaderPanel headerPanel;
    private final GridCanvasPanel gridCanvasPanel;
    private final RightSidebarPanel rightSidebarPanel;
    private final ControlPanel controlPanel;
    private final JPanel centerContainer;

    private File mapFile = null;
    private File commandFile = null;
    private SimulationManager simulationManager = null;

    public MainFrame() {
        super("Gezegen Postacıları - Simülasyon");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setMinimumSize(new Dimension(1200, 850));
        setPreferredSize(new Dimension(1350, 920));
        setLocationRelativeTo(null);
        getContentPane().setBackground(UITheme.BG_SPACE);
        setLayout(new BorderLayout());

        headerPanel = new HeaderPanel();
        headerPanel.setFileLoadListener(new HeaderPanel.FileLoadListener() {
            @Override
            public void onMapFileSelected(File file) {
                mapFile = file;
                tryLoadMapOnly(file);
            }

            @Override
            public void onCommandFileSelected(File file) {
                commandFile = file;
                if (mapFile != null) {
                    loadSimulation(mapFile, commandFile);
                } else {
                    controlPanel.setStatusMessage("Komutlar seçildi. Lütfen harita dosyasını da seçiniz.");
                }
            }
        });
        add(headerPanel, BorderLayout.NORTH);

        gridCanvasPanel = new GridCanvasPanel();

        centerContainer = new JPanel(new BorderLayout());
        centerContainer.setBackground(UITheme.BG_SPACE);
        centerContainer.add(gridCanvasPanel, BorderLayout.CENTER);
        add(centerContainer, BorderLayout.CENTER);

        rightSidebarPanel = new RightSidebarPanel();
        add(rightSidebarPanel, BorderLayout.EAST);

        controlPanel = new ControlPanel();
        controlPanel.setControlListener(new ControlPanel.ControlEventListener() {
            @Override
            public void onStepForward() {
                if (simulationManager != null) {
                    boolean hasMore = simulationManager.stepForward();
                    syncUIWithSimulation();
                    if (!hasMore) {
                        controlPanel.stopSimulation();
                        ConsoleOutputFormatter.printReport(simulationManager.getGrid());
                    }
                }
            }

            @Override
            public void onStepBackward() {
                if (simulationManager != null) {
                    simulationManager.stepBackward();
                    syncUIWithSimulation();
                }
            }

            @Override
            public void onReset() {
                if (mapFile != null && commandFile != null) {
                    loadSimulation(mapFile, commandFile);
                } else {
                    gridCanvasPanel.setGrid(gridCanvasPanel.getGrid());
                    syncUIWithSimulation();
                }
            }

            @Override
            public void onEnd() {
                if (simulationManager != null) {
                    simulationManager.runAll();
                    syncUIWithSimulation();
                    controlPanel.stopSimulation();
                    ConsoleOutputFormatter.printReport(simulationManager.getGrid());
                }
            }

            @Override
            public void onPlayStateChanged(boolean playing) {
                if (simulationManager == null && playing) {
                    controlPanel.stopSimulation();
                    JOptionPane.showMessageDialog(MainFrame.this,
                            "Simülasyonu oynatmak için lütfen üst kısımdan Harita ve Komutlar dosyalarını yükleyiniz.",
                            "Dosya Gerekli", JOptionPane.INFORMATION_MESSAGE);
                }
            }

            @Override
            public void onSpeedChanged(int delayMs) {
            }
        });
        add(controlPanel, BorderLayout.SOUTH);

        updateInitialMetrics();

        pack();
    }

    private void tryLoadMapOnly(File file) {
        try {
            MapFileReader reader = new MapFileReader();
            MapFileReader.ParseResult result = reader.read(file);
            gridCanvasPanel.setGrid(result.getGrid());
            rightSidebarPanel.getRobotStatusPanel().updateRobotList(result.getGrid().getRobots());
            controlPanel.setStatusMessage("Harita yüklendi (" + result.getGrid().getWidth() + "x" + result.getGrid().getHeight() + "). Şimdi komutlar dosyasını seçin.");
            updateInitialMetrics();
        } catch (InvalidMapException | IOException ex) {
            JOptionPane.showMessageDialog(this, ex.getMessage(), "Harita Hatası", JOptionPane.ERROR_MESSAGE);
            controlPanel.setStatusMessage("HATA: " + ex.getMessage());
        }
    }

    public void loadSimulation(File mFile, File cFile) {
        this.mapFile = mFile;
        this.commandFile = cFile;

        headerPanel.setMapFile(mFile);
        headerPanel.setCommandFile(cFile);

        try {
            MapFileReader mapReader = new MapFileReader();
            MapFileReader.ParseResult mapResult = mapReader.read(mFile);

            CommandFileReader cmdReader = new CommandFileReader();
            List<Command> commands = cmdReader.read(cFile);

            this.simulationManager = new SimulationManager(mapResult.getGrid(), commands);

            gridCanvasPanel.setGrid(simulationManager.getGrid());
            syncUIWithSimulation();
            controlPanel.setStatusMessage("Simülasyon hazır (" + commands.size() + " komut yüklendi). 'İleri' veya 'Oynat' ile başlatabilirsiniz.");

        } catch (InvalidMapException | InvalidCommandException ex) {
            JOptionPane.showMessageDialog(this, ex.getMessage(), "Dosya Format Hatası", JOptionPane.ERROR_MESSAGE);
            controlPanel.setStatusMessage("HATA: " + ex.getMessage());
            System.err.println(ex.getMessage());
        } catch (IOException ex) {
            JOptionPane.showMessageDialog(this, "Dosya okunamadı: " + ex.getMessage(), "Okuma Hatası", JOptionPane.ERROR_MESSAGE);
            controlPanel.setStatusMessage("HATA: " + ex.getMessage());
            System.err.println(ex.getMessage());
        }
    }

    private void syncUIWithSimulation() {
        if (simulationManager == null) return;

        gridCanvasPanel.repaint();

        rightSidebarPanel.getRobotStatusPanel().updateRobotList(simulationManager.getGrid().getRobots());

        int currentCmd = Math.min(simulationManager.getCommandIndex() + 1, simulationManager.getTotalCommands());
        int currentStep = simulationManager.getTotalStepsCompleted();
        int totalExpected = simulationManager.getTotalExpectedSteps();
        int delivered = simulationManager.getDeliveredCount();
        int totalRobots = simulationManager.getGrid().getRobots().size();
        int remainingEnergy = simulationManager.getTotalRemainingEnergy();

        controlPanel.updateMetrics(currentStep, totalExpected, currentCmd, simulationManager.getTotalCommands(),
                delivered, totalRobots, remainingEnergy);

        controlPanel.setStatusMessage(simulationManager.getLastActionMessage());
    }

    private void updateInitialMetrics() {
        Grid grid = gridCanvasPanel.getGrid();
        if (grid != null) {
            List<Robot> robots = grid.getRobots();
            int totalEnergy = 0;
            for (Robot r : robots) {
                totalEnergy += r.getEnergy();
            }
            controlPanel.updateMetrics(0, 0, 0, 0, 0, robots.size(), totalEnergy);
        }
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            MainFrame frame = new MainFrame();
            File defaultMap = new File("harita.txt");
            if (!defaultMap.exists()) defaultMap = new File("src/harita.txt");
            File defaultCmd = new File("komutlar.txt");
            if (!defaultCmd.exists()) defaultCmd = new File("src/komutlar.txt");

            if (defaultMap.exists() && defaultCmd.exists()) {
                frame.loadSimulation(defaultMap, defaultCmd);
            }
            frame.setVisible(true);
        });
    }
}
