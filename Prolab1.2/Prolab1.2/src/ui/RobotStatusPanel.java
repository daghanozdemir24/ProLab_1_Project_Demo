package ui;

import model.common.RobotStatus;
import model.robot.Robot;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.util.ArrayList;
import java.util.List;

public class RobotStatusPanel extends JPanel {

    private final JPanel cardsContainer;
    private final List<RobotCardComponent> cardComponents = new ArrayList<>();

    public RobotStatusPanel() {
        setLayout(new BorderLayout(0, 10));
        setBackground(UITheme.PANEL_BG);
        setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(UITheme.PANEL_BORDER, 1),
            BorderFactory.createEmptyBorder(12, 14, 12, 14)
        ));

        JLabel headerLabel = new JLabel("Robotlar");
        headerLabel.setFont(UITheme.FONT_BUTTON);
        headerLabel.setForeground(UITheme.TEXT_PRIMARY);
        add(headerLabel, BorderLayout.NORTH);

        cardsContainer = new JPanel();
        cardsContainer.setLayout(new BoxLayout(cardsContainer, BoxLayout.Y_AXIS));
        cardsContainer.setOpaque(false);

        initDefaultCards();

        add(cardsContainer, BorderLayout.CENTER);
    }

    private void initDefaultCards() {
        cardComponents.clear();
        cardsContainer.removeAll();

        cardComponents.add(new RobotCardComponent(1, "Kızaklı", "kayabilir", 20, RobotStatus.AKTIF, UITheme.ROBOT_1));
        cardComponents.add(new RobotCardComponent(2, "Paletli", "özel yeteneği yok", 20, RobotStatus.AKTIF, UITheme.ROBOT_2));
        cardComponents.add(new RobotCardComponent(3, "Havalı", "uçabilir", 20, RobotStatus.AKTIF, UITheme.ROBOT_3));
        cardComponents.add(new RobotCardComponent(4, "Sıçrayan", "atlayabilir", 20, RobotStatus.AKTIF, UITheme.ROBOT_4));
        cardComponents.add(new RobotCardComponent(5, "Zırhlı", "çamurda korumalı", 20, RobotStatus.AKTIF, UITheme.ROBOT_5));

        for (RobotCardComponent card : cardComponents) {
            cardsContainer.add(card);
            cardsContainer.add(Box.createVerticalStrut(8));
        }
    }

    public void updateRobotList(List<Robot> robots) {
        if (robots == null) return;

        for (Robot r : robots) {
            for (RobotCardComponent card : cardComponents) {
                if (card.getRobotId() == r.getId()) {
                    card.updateState(r.getEnergy(), r.getStatus());
                    break;
                }
            }
        }
        revalidate();
        repaint();
    }

    private static class RobotCardComponent extends JPanel {
        private final int robotId;
        private final JLabel energyLabel;
        private final JLabel statusBadge;

        public RobotCardComponent(int id, String bodyType, String capability, int initialEnergy, RobotStatus initialStatus, Color color) {
            this.robotId = id;
            setLayout(new BorderLayout(8, 0));
            setBackground(UITheme.BG_SPACE);
            setBorder(BorderFactory.createCompoundBorder(
                BorderFactory.createLineBorder(UITheme.PANEL_BORDER, 1),
                BorderFactory.createEmptyBorder(7, 10, 7, 10)
            ));

            JPanel badgeIcon = new JPanel() {
                @Override
                protected void paintComponent(Graphics g) {
                    super.paintComponent(g);
                    Graphics2D g2 = (Graphics2D) g.create();
                    g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                    g2.setColor(color);
                    g2.fillOval(0, 0, getWidth(), getHeight());
                    g2.setColor(Color.WHITE);
                    g2.setFont(new java.awt.Font("Segoe UI", java.awt.Font.BOLD, 10));
                    String text = "R" + id;
                    java.awt.FontMetrics fm = g2.getFontMetrics();
                    g2.drawString(text, (getWidth() - fm.stringWidth(text)) / 2, (getHeight() + fm.getAscent() - fm.getDescent()) / 2);
                    g2.dispose();
                }
            };
            badgeIcon.setPreferredSize(new Dimension(22, 22));
            badgeIcon.setOpaque(false);

            JPanel iconContainer = new JPanel(new FlowLayout(FlowLayout.CENTER, 0, 4));
            iconContainer.setOpaque(false);
            iconContainer.add(badgeIcon);

            JPanel centerText = new JPanel();
            centerText.setLayout(new BoxLayout(centerText, BoxLayout.Y_AXIS));
            centerText.setOpaque(false);

            JLabel nameLabel = new JLabel("R" + id + " " + bodyType);
            nameLabel.setFont(UITheme.FONT_BUTTON);
            nameLabel.setForeground(UITheme.TEXT_PRIMARY);

            JLabel capLabel = new JLabel(capability);
            capLabel.setFont(new java.awt.Font("Segoe UI", java.awt.Font.PLAIN, 10));
            capLabel.setForeground(UITheme.TEXT_MUTED);

            centerText.add(nameLabel);
            centerText.add(Box.createVerticalStrut(2));
            centerText.add(capLabel);

            JPanel rightContainer = new JPanel();
            rightContainer.setLayout(new BoxLayout(rightContainer, BoxLayout.Y_AXIS));
            rightContainer.setOpaque(false);

            energyLabel = new JLabel("enerji " + initialEnergy);
            energyLabel.setFont(new java.awt.Font("Segoe UI", java.awt.Font.BOLD, 11));
            energyLabel.setForeground(UITheme.TEXT_SECONDARY);
            energyLabel.setAlignmentX(1.0f);

            statusBadge = new JLabel();
            statusBadge.setFont(new java.awt.Font("Segoe UI", java.awt.Font.BOLD, 10));
            statusBadge.setAlignmentX(1.0f);

            updateState(initialEnergy, initialStatus);

            rightContainer.add(energyLabel);
            rightContainer.add(Box.createVerticalStrut(3));
            rightContainer.add(statusBadge);

            add(iconContainer, BorderLayout.WEST);
            add(centerText, BorderLayout.CENTER);
            add(rightContainer, BorderLayout.EAST);
        }

        public int getRobotId() {
            return robotId;
        }

        public void updateState(int energy, RobotStatus status) {
            energyLabel.setText("enerji " + energy);

            if (status == RobotStatus.TESLIM) {
                statusBadge.setText("paketi teslim etti");
                statusBadge.setForeground(UITheme.GREEN_PRIMARY);
            } else if (status == RobotStatus.ENERJISIZ) {
                statusBadge.setText("enerjisiz");
                statusBadge.setForeground(new Color(239, 68, 68));
            } else {
                statusBadge.setText("yolda");
                statusBadge.setForeground(new Color(56, 189, 248));
            }
        }
    }
}
