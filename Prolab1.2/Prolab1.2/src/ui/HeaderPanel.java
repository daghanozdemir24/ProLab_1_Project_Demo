package ui;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.filechooser.FileNameExtensionFilter;
import java.awt.BorderLayout;
import java.awt.Cursor;
import java.awt.FlowLayout;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;

public class HeaderPanel extends JPanel {

    private final JLabel titleLabel;
    private final JLabel subtitleLabel;

    private File selectedMapFile = null;
    private File selectedCommandFile = null;

    private final JLabel mapStatusBadge;
    private final JLabel commandStatusBadge;
    private final JButton loadMapButton;
    private final JButton loadCommandButton;

    public interface FileLoadListener {
        void onMapFileSelected(File file);
        void onCommandFileSelected(File file);
    }

    private FileLoadListener fileLoadListener;

    public HeaderPanel() {
        setLayout(new BorderLayout());
        setBackground(UITheme.BG_SPACE);
        setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createMatteBorder(0, 0, 1, 0, UITheme.PANEL_BORDER),
            BorderFactory.createEmptyBorder(14, 20, 14, 20)
        ));

        JPanel textContainer = new JPanel();
        textContainer.setLayout(new BoxLayout(textContainer, BoxLayout.Y_AXIS));
        textContainer.setOpaque(false);

        titleLabel = new JLabel("Gezegen Postacıları");
        titleLabel.setFont(UITheme.FONT_TITLE);
        titleLabel.setForeground(UITheme.TEXT_PRIMARY);

        subtitleLabel = new JLabel("Aynı komut listesi, dört farklı robot gövdesi. Amaç: paketleri sağ kenardaki teslim rampasına ulaştırmak.");
        subtitleLabel.setFont(UITheme.FONT_SUBTITLE);
        subtitleLabel.setForeground(UITheme.TEXT_SECONDARY);

        textContainer.add(titleLabel);
        textContainer.add(Box.createVerticalStrut(4));
        textContainer.add(subtitleLabel);

        add(textContainer, BorderLayout.WEST);

        JPanel fileControlsPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT, 12, 0));
        fileControlsPanel.setOpaque(false);

        JPanel mapGroup = createFileInputGroup("Harita (.txt)", "Harita seçilmedi");
        loadMapButton = (JButton) mapGroup.getClientProperty("button");
        mapStatusBadge = (JLabel) mapGroup.getClientProperty("badge");

        JPanel commandGroup = createFileInputGroup("Komutlar (.txt)", "Komut seçilmedi");
        loadCommandButton = (JButton) commandGroup.getClientProperty("button");
        commandStatusBadge = (JLabel) commandGroup.getClientProperty("badge");

        loadMapButton.addActionListener(e -> chooseMapFile());
        loadCommandButton.addActionListener(e -> chooseCommandFile());

        fileControlsPanel.add(mapGroup);
        fileControlsPanel.add(commandGroup);

        add(fileControlsPanel, BorderLayout.EAST);
    }

    private JPanel createFileInputGroup(String buttonText, String initialBadgeText) {
        JPanel group = new JPanel();
        group.setLayout(new BoxLayout(group, BoxLayout.Y_AXIS));
        group.setOpaque(false);

        JButton button = new JButton(buttonText);
        button.setFont(UITheme.FONT_BUTTON);
        button.setForeground(UITheme.TEXT_PRIMARY);
        button.setBackground(UITheme.GREEN_DARK);
        button.setFocusPainted(false);
        button.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(UITheme.GREEN_BORDER, 1),
            BorderFactory.createEmptyBorder(6, 14, 6, 14)
        ));
        button.setCursor(new Cursor(Cursor.HAND_CURSOR));

        button.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseEntered(MouseEvent e) {
                button.setBackground(UITheme.GREEN_HOVER);
            }

            @Override
            public void mouseExited(MouseEvent e) {
                button.setBackground(UITheme.GREEN_DARK);
            }
        });

        JLabel badge = new JLabel(initialBadgeText);
        badge.setFont(UITheme.FONT_BADGE);
        badge.setForeground(UITheme.TEXT_MUTED);
        badge.setAlignmentX(0.5f);

        group.add(button);
        group.add(Box.createVerticalStrut(4));
        group.add(badge);

        group.putClientProperty("button", button);
        group.putClientProperty("badge", badge);

        return group;
    }

    private void chooseMapFile() {
        JFileChooser fileChooser = createFileChooser("Harita Dosyası Seç (harita.txt)");
        int result = fileChooser.showOpenDialog(this);
        if (result == JFileChooser.APPROVE_OPTION) {
            this.selectedMapFile = fileChooser.getSelectedFile();
            mapStatusBadge.setText(selectedMapFile.getName());
            mapStatusBadge.setForeground(UITheme.GREEN_PRIMARY);
            if (fileLoadListener != null) {
                fileLoadListener.onMapFileSelected(selectedMapFile);
            }
        }
    }

    private void chooseCommandFile() {
        JFileChooser fileChooser = createFileChooser("Komut Dosyası Seç (komutlar.txt)");
        int result = fileChooser.showOpenDialog(this);
        if (result == JFileChooser.APPROVE_OPTION) {
            this.selectedCommandFile = fileChooser.getSelectedFile();
            commandStatusBadge.setText(selectedCommandFile.getName());
            commandStatusBadge.setForeground(UITheme.GREEN_PRIMARY);
            if (fileLoadListener != null) {
                fileLoadListener.onCommandFileSelected(selectedCommandFile);
            }
        }
    }

    public void setMapFile(File file) {
        this.selectedMapFile = file;
        if (file != null) {
            mapStatusBadge.setText(file.getName());
            mapStatusBadge.setForeground(UITheme.GREEN_PRIMARY);
        } else {
            mapStatusBadge.setText("Harita seçilmedi");
            mapStatusBadge.setForeground(UITheme.TEXT_MUTED);
        }
    }

    public void setCommandFile(File file) {
        this.selectedCommandFile = file;
        if (file != null) {
            commandStatusBadge.setText(file.getName());
            commandStatusBadge.setForeground(UITheme.GREEN_PRIMARY);
        } else {
            commandStatusBadge.setText("Komut seçilmedi");
            commandStatusBadge.setForeground(UITheme.TEXT_MUTED);
        }
    }

    private JFileChooser createFileChooser(String dialogTitle) {
        JFileChooser chooser = new JFileChooser(new File("."));
        chooser.setDialogTitle(dialogTitle);
        chooser.setFileFilter(new FileNameExtensionFilter("Metin Dosyaları (*.txt)", "txt"));
        chooser.setAcceptAllFileFilterUsed(false);
        return chooser;
    }

    public void setFileLoadListener(FileLoadListener listener) {
        this.fileLoadListener = listener;
    }

    public File getSelectedMapFile() {
        return selectedMapFile;
    }

    public File getSelectedCommandFile() {
        return selectedCommandFile;
    }
}
