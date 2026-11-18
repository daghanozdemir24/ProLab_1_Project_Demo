package ui;

import model.ground.*;

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
import java.awt.GridLayout;
import java.awt.RenderingHints;

public class LegendPanel extends JPanel {

    public LegendPanel() {
        setLayout(new BorderLayout(0, 10));
        setBackground(UITheme.PANEL_BG);
        setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(UITheme.PANEL_BORDER, 1),
            BorderFactory.createEmptyBorder(12, 14, 12, 14)
        ));

        JLabel headerLabel = new JLabel("Zemin Rehberi");
        headerLabel.setFont(UITheme.FONT_BUTTON);
        headerLabel.setForeground(UITheme.TEXT_PRIMARY);
        add(headerLabel, BorderLayout.NORTH);

        JPanel gridPanel = new JPanel(new GridLayout(4, 2, 8, 8));
        gridPanel.setOpaque(false);

        gridPanel.add(createLegendCard("Düz Zemin", "Tüm gövdeler 1 enerji", new PlainGround()));
        gridPanel.add(createLegendCard("Toz Fırtınası", "2 enerji (Havalı savrulur)", new DustStorm()));
        gridPanel.add(createLegendCard("Kraterli Kaya", "3 enerji (Sıçrayan atlar 2)", new CraterRock()));
        gridPanel.add(createLegendCard("Metan Gölü", "Geçilmez (Havalı 1)", new MethaneLake()));
        gridPanel.add(createLegendCard("Buz Kabuğu", "Kızaklı kayar (1), Diğer 2", new IceCrust()));
        gridPanel.add(createLegendCard("Engel (X)", "Hiçbir robot geçemez", new Obstacle()));
        gridPanel.add(createLegendCard("Teslim Rampası", "Paket teslimi (1)", new DeliveryRamp()));
        gridPanel.add(createLegendCard("Çamur Arazisi", "Zırhlı 2, Havalı 1, Diğer 4", new MuddyPlace()));

        add(gridPanel, BorderLayout.CENTER);
    }

    private JPanel createLegendCard(String title, String rule, Ground ground) {
        JPanel card = new JPanel(new BorderLayout(8, 0));
        card.setBackground(UITheme.BG_SPACE);
        card.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(UITheme.PANEL_BORDER, 1),
            BorderFactory.createEmptyBorder(6, 8, 6, 8)
        ));

        JPanel colorBox = new JPanel() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                Graphics2D g2 = (Graphics2D) g.create();
                GridCanvasPanel.renderGroundTile(g2, ground, 0, 0, getWidth());
                g2.dispose();
            }
        };
        colorBox.setPreferredSize(new Dimension(18, 18));
        colorBox.setOpaque(false);

        JPanel leftContainer = new JPanel(new FlowLayout(FlowLayout.CENTER, 0, 4));
        leftContainer.setOpaque(false);
        leftContainer.add(colorBox);

        JPanel textContainer = new JPanel();
        textContainer.setLayout(new BoxLayout(textContainer, BoxLayout.Y_AXIS));
        textContainer.setOpaque(false);

        JLabel titleLbl = new JLabel(title);
        titleLbl.setFont(UITheme.FONT_BADGE);
        titleLbl.setForeground(UITheme.TEXT_PRIMARY);

        JLabel ruleLbl = new JLabel(rule);
        ruleLbl.setFont(new java.awt.Font("Segoe UI", java.awt.Font.PLAIN, 10));
        ruleLbl.setForeground(UITheme.TEXT_MUTED);

        textContainer.add(titleLbl);
        textContainer.add(Box.createVerticalStrut(2));
        textContainer.add(ruleLbl);

        card.add(leftContainer, BorderLayout.WEST);
        card.add(textContainer, BorderLayout.CENTER);

        return card;
    }
}
