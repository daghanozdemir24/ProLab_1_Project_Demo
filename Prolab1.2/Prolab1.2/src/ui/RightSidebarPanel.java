package ui;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import java.awt.BorderLayout;
import java.awt.Dimension;

public class RightSidebarPanel extends JPanel {

    private final LegendPanel legendPanel;
    private final RobotStatusPanel robotStatusPanel;

    public RightSidebarPanel() {
        setLayout(new BorderLayout(0, 12));
        setBackground(UITheme.BG_SPACE);
        setBorder(BorderFactory.createEmptyBorder(10, 0, 10, 15));
        setPreferredSize(new Dimension(380, 0));

        legendPanel = new LegendPanel();
        add(legendPanel, BorderLayout.NORTH);

        robotStatusPanel = new RobotStatusPanel();
        add(robotStatusPanel, BorderLayout.CENTER);
    }

    public LegendPanel getLegendPanel() {
        return legendPanel;
    }

    public RobotStatusPanel getRobotStatusPanel() {
        return robotStatusPanel;
    }
}
