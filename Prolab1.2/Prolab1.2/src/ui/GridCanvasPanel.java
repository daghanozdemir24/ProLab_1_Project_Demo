package ui;

import engine.Grid;
import model.common.Direction;
import model.common.Position;
import model.common.RobotStatus;
import model.ground.CraterRock;
import model.ground.DeliveryRamp;
import model.ground.DustStorm;
import model.ground.Ground;
import model.ground.IceCrust;
import model.ground.MethaneLake;
import model.ground.MuddyPlace;
import model.ground.Obstacle;
import model.ground.PlainGround;
import model.robot.FlyingRobot;
import model.robot.JumpingRobot;
import model.robot.Robot;
import model.robot.SlidingRobot;
import model.robot.TrackedRobot;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import java.awt.*;
import java.util.List;

public class GridCanvasPanel extends JPanel {

    private Grid grid;

    private static final int COORD_MARGIN = 26;
    private static final int MIN_CELL_SIZE = 28;
    private static final int MAX_CELL_SIZE = 72;

    public GridCanvasPanel() {
        setBackground(UITheme.BG_SPACE);
        setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        this.grid = createSampleGrid();
    }

    public void setGrid(Grid grid) {
        this.grid = grid;
        repaint();
    }

    public Grid getGrid() {
        return grid;
    }

    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);

        if (grid == null) {
            return;
        }

        Graphics2D g2d = (Graphics2D) g.create();
        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g2d.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);

        int cols = grid.getWidth();
        int rows = grid.getHeight();

        if (cols <= 0 || rows <= 0) {
            g2d.dispose();
            return;
        }

        int availableW = getWidth() - COORD_MARGIN - 20;
        int availableH = getHeight() - COORD_MARGIN - 20;

        int cellSize = Math.min(availableW / cols, availableH / rows);
        cellSize = Math.max(MIN_CELL_SIZE, Math.min(cellSize, MAX_CELL_SIZE));

        int totalGridW = cols * cellSize;
        int totalGridH = rows * cellSize;
        int startX = COORD_MARGIN + Math.max(0, (availableW - totalGridW) / 2);
        int startY = COORD_MARGIN + Math.max(0, (availableH - totalGridH) / 2);

        drawCoordinates(g2d, cols, rows, startX, startY, cellSize);
        drawGridCells(g2d, cols, rows, startX, startY, cellSize);
        drawRobots(g2d, startX, startY, cellSize);

        g2d.dispose();
    }

    private void drawCoordinates(Graphics2D g2, int cols, int rows, int startX, int startY, int cellSize) {
        g2.setFont(UITheme.FONT_COORDINATE);
        g2.setColor(UITheme.TEXT_MUTED);

        FontMetrics fm = g2.getFontMetrics();

        for (int x = 0; x < cols; x++) {
            String text = String.valueOf(x);
            int textW = fm.stringWidth(text);
            int cellCenterX = startX + (x * cellSize) + (cellSize / 2);
            g2.drawString(text, cellCenterX - (textW / 2), startY - 8);
        }

        for (int y = 0; y < rows; y++) {
            String text = String.valueOf(y);
            int textW = fm.stringWidth(text);
            int cellCenterY = startY + (y * cellSize) + (cellSize / 2) + (fm.getAscent() / 2) - 2;
            g2.drawString(text, startX - textW - 8, cellCenterY);
        }
    }

    private void drawGridCells(Graphics2D g2, int cols, int rows, int startX, int startY, int cellSize) {
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                int px = startX + (x * cellSize);
                int py = startY + (y * cellSize);

                Ground ground = grid.getGroundAt(new Position(x, y));
                renderGroundTile(g2, ground, px, py, cellSize);
            }
        }
    }

    private void drawRobots(Graphics2D g2, int startX, int startY, int cellSize) {
        List<Robot> robots = grid.getRobots();
        if (robots == null) return;

        int badgeDiameter = (int) (cellSize * 0.72);
        int badgeOffset = (cellSize - badgeDiameter) / 2;

        for (Robot r : robots) {
            if (r.getStatus() == RobotStatus.TESLIM) {
                continue;
            }

            Position pos = r.getPosition();
            if (pos == null) continue;

            int cellX = startX + (pos.getX() * cellSize);
            int cellY = startY + (pos.getY() * cellSize);
            int circleX = cellX + badgeOffset;
            int circleY = cellY + badgeOffset;

            Color robotColor = getRobotColor(r.getId());

            g2.setColor(new Color(0, 0, 0, 80));
            g2.fillOval(circleX + 2, circleY + 2, badgeDiameter, badgeDiameter);

            g2.setColor(robotColor);
            g2.fillOval(circleX, circleY, badgeDiameter, badgeDiameter);

            if (r.getStatus() == RobotStatus.ENERJISIZ) {
                g2.setColor(new Color(239, 68, 68));
                g2.setStroke(new BasicStroke(2.5f));
                g2.drawOval(circleX, circleY, badgeDiameter, badgeDiameter);
                g2.setStroke(new BasicStroke(1.0f));
            } else {
                g2.setColor(new Color(255, 255, 255, 140));
                g2.setStroke(new BasicStroke(1.2f));
                g2.drawOval(circleX, circleY, badgeDiameter, badgeDiameter);
                g2.setStroke(new BasicStroke(1.0f));
            }

            g2.setColor(Color.WHITE);
            g2.setFont(UITheme.FONT_ROBOT_BADGE);
            FontMetrics fm = g2.getFontMetrics();
            String label = "R" + r.getId();
            int textX = circleX + (badgeDiameter - fm.stringWidth(label)) / 2;
            int textY = circleY + (badgeDiameter + fm.getAscent() - fm.getDescent()) / 2 - 1;
            g2.drawString(label, textX, textY);

            drawDirectionIndicator(g2, r.getDirection(), cellX, cellY, cellSize, badgeDiameter);
        }
    }

    private void drawDirectionIndicator(Graphics2D g2, Direction dir, int cellX, int cellY, int cellSize, int badgeDiameter) {
        if (dir == null) return;

        int centerX = cellX + (cellSize / 2);
        int centerY = cellY + (cellSize / 2);
        int arrowRadius = (badgeDiameter / 2) + 5;

        int tipX = centerX;
        int tipY = centerY;
        int base1X = centerX, base1Y = centerY;
        int base2X = centerX, base2Y = centerY;

        int arrowSize = 4;

        switch (dir) {
            case N:
                tipX = centerX;
                tipY = centerY - arrowRadius;
                base1X = tipX - arrowSize;
                base1Y = tipY + (arrowSize * 2);
                base2X = tipX + arrowSize;
                base2Y = tipY + (arrowSize * 2);
                break;
            case S:
                tipX = centerX;
                tipY = centerY + arrowRadius;
                base1X = tipX - arrowSize;
                base1Y = tipY - (arrowSize * 2);
                base2X = tipX + arrowSize;
                base2Y = tipY - (arrowSize * 2);
                break;
            case E:
                tipX = centerX + arrowRadius;
                tipY = centerY;
                base1X = tipX - (arrowSize * 2);
                base1Y = tipY - arrowSize;
                base2X = tipX - (arrowSize * 2);
                base2Y = tipY + arrowSize;
                break;
            case W:
                tipX = centerX - arrowRadius;
                tipY = centerY;
                base1X = tipX + (arrowSize * 2);
                base1Y = tipY - arrowSize;
                base2X = tipX + (arrowSize * 2);
                base2Y = tipY + arrowSize;
                break;
        }

        Polygon arrow = new Polygon(new int[]{tipX, base1X, base2X}, new int[]{tipY, base1Y, base2Y}, 3);
        g2.setColor(Color.WHITE);
        g2.fillPolygon(arrow);
    }

    public static void renderGroundTile(Graphics2D g2, Ground ground, int px, int py, int size) {
        Graphics2D g = (Graphics2D) g2.create();
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        Color baseColor = getGroundColor(ground);
        g.setColor(baseColor);
        g.fillRect(px, py, size, size);

        if (ground instanceof DustStorm) {
            g.setColor(new Color(245, 195, 85, 160));
            g.setStroke(new BasicStroke(Math.max(1.0f, size * 0.04f)));
            int cy = py + size / 2;
            g.drawArc(px + 3, cy - size / 4, size - 6, size / 2, 0, 160);
            g.drawArc(px + 6, cy - size / 6, size - 12, size / 3, 180, 160);

            g.setColor(new Color(254, 240, 138, 220));
            int dotSize = Math.max(2, size / 16);
            g.fillRect(px + size / 4, py + size / 5, dotSize, dotSize);
            g.fillRect(px + (size * 3) / 4, py + size / 3, dotSize, dotSize);
            g.fillRect(px + (size * 3) / 5, py + (size * 3) / 4, dotSize, dotSize);

        } else if (ground instanceof CraterRock) {
            int c1x = px + (int)(size * 0.18);
            int c1y = py + (int)(size * 0.20);
            int c1d = (int)(size * 0.50);
            g.setColor(new Color(115, 92, 80));
            g.drawOval(c1x, c1y, c1d, c1d);
            g.setColor(new Color(36, 26, 22));
            g.fillOval(c1x + 1, c1y + 1, c1d - 2, c1d - 2);
            g.setColor(new Color(155, 128, 112, 190));
            g.drawArc(c1x, c1y, c1d, c1d, 45, 135);

            int c2x = px + (int)(size * 0.65);
            int c2y = py + (int)(size * 0.62);
            int c2d = (int)(size * 0.24);
            g.setColor(new Color(115, 92, 80));
            g.drawOval(c2x, c2y, c2d, c2d);
            g.setColor(new Color(36, 26, 22));
            g.fillOval(c2x + 1, c2y + 1, c2d - 2, c2d - 2);

        } else if (ground instanceof MethaneLake) {
            g.setColor(new Color(56, 189, 248, 220));
            g.setStroke(new BasicStroke(Math.max(1.4f, size * 0.05f), BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND));

            int w1y = py + (int)(size * 0.35);
            int w2y = py + (int)(size * 0.65);
            drawWave(g, px + 2, w1y, size - 4, Math.max(2, size / 12));
            drawWave(g, px + 4, w2y, size - 8, Math.max(2, size / 12));

        } else if (ground instanceof IceCrust) {
            g.setColor(new Color(224, 242, 254, 210));
            g.setStroke(new BasicStroke(Math.max(1.2f, size * 0.04f)));
            g.drawLine(px + 3, py + (int)(size * 0.28), px + (int)(size * 0.45), py + (int)(size * 0.52));
            g.drawLine(px + (int)(size * 0.45), py + (int)(size * 0.52), px + size - 4, py + (int)(size * 0.38));
            g.drawLine(px + (int)(size * 0.45), py + (int)(size * 0.52), px + (int)(size * 0.52), py + size - 4);

            g.setColor(new Color(255, 255, 255, 80));
            g.fillPolygon(new int[]{px + 1, px + (int)(size * 0.35), px + 1},
                          new int[]{py + 1, py + 1, py + (int)(size * 0.35)}, 3);

        } else if (ground instanceof Obstacle) {
            g.setColor(new Color(239, 68, 68, 220));
            g.setStroke(new BasicStroke(Math.max(2.2f, size * 0.08f), BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND));
            int pad = (int)(size * 0.22);
            g.drawLine(px + pad, py + pad, px + size - pad, py + size - pad);
            g.drawLine(px + size - pad, py + pad, px + pad, py + size - pad);

        } else if (ground instanceof DeliveryRamp) {
            g.setColor(new Color(52, 211, 153, 240));
            g.setStroke(new BasicStroke(Math.max(2.0f, size * 0.07f), BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND));
            int midY = py + size / 2;
            int h = (int)(size * 0.25);
            int o1x = px + (int)(size * 0.35);
            int o2x = px + (int)(size * 0.65);
            int tip = Math.max(3, size / 8);

            g.drawLine(o1x - tip, midY - h, o1x + tip, midY);
            g.drawLine(o1x + tip, midY, o1x - tip, midY + h);

            g.drawLine(o2x - tip, midY - h, o2x + tip, midY);
            g.drawLine(o2x + tip, midY, o2x - tip, midY + h);

        } else if (ground instanceof MuddyPlace) {
            g.setColor(new Color(110, 80, 55, 180));
            g.setStroke(new BasicStroke(1.2f));
            g.drawOval(px + (int)(size * 0.20), py + (int)(size * 0.28), (int)(size * 0.36), (int)(size * 0.26));
            g.drawOval(px + (int)(size * 0.52), py + (int)(size * 0.50), (int)(size * 0.30), (int)(size * 0.24));
            g.setColor(new Color(40, 28, 20));
            g.fillOval(px + (int)(size * 0.32), py + (int)(size * 0.36), 3, 3);
            g.fillOval(px + (int)(size * 0.62), py + (int)(size * 0.58), 2, 2);
        }

        g.setColor(UITheme.GRID_LINE);
        g.setStroke(new BasicStroke(1.0f));
        g.drawRect(px, py, size, size);

        g.dispose();
    }

    private static void drawWave(Graphics2D g, int x, int y, int width, int waveHeight) {
        int segments = 4;
        int segW = width / segments;
        int curX = x;
        for (int i = 0; i < segments; i++) {
            if (i % 2 == 0) {
                g.drawArc(curX, y - waveHeight, segW, waveHeight * 2, 0, 180);
            } else {
                g.drawArc(curX, y - waveHeight, segW, waveHeight * 2, 180, 180);
            }
            curX += segW;
        }
    }

    public static Color getGroundColor(Ground ground) {
        if (ground instanceof PlainGround) return UITheme.GROUND_PLAIN;
        if (ground instanceof DustStorm) return UITheme.GROUND_DUST;
        if (ground instanceof CraterRock) return UITheme.GROUND_CRATER;
        if (ground instanceof MethaneLake) return UITheme.GROUND_METHANE;
        if (ground instanceof IceCrust) return UITheme.GROUND_ICE;
        if (ground instanceof Obstacle) return UITheme.GROUND_OBSTACLE;
        if (ground instanceof DeliveryRamp) return UITheme.GROUND_DELIVERY;
        if (ground instanceof MuddyPlace) return UITheme.GROUND_MUD;
        return UITheme.GROUND_PLAIN;
    }

    private Color getRobotColor(int robotId) {
        switch (robotId) {
            case 1: return UITheme.ROBOT_1;
            case 2: return UITheme.ROBOT_2;
            case 3: return UITheme.ROBOT_3;
            case 4: return UITheme.ROBOT_4;
            case 5: return UITheme.ROBOT_5;
            default: return UITheme.GREEN_PRIMARY;
        }
    }

    private Grid createSampleGrid() {
        int width = 12;
        int height = 8;
        Grid sampleGrid = new Grid(width, height);

        String[] mapLines = {
            "1.I.TTTT.IIH",
            "..IK.......H",
            "2.IK.IIX...H",
            "..I.K......H",
            "3...K..III.H",
            ".........T.H",
            "4.~~~~IIII.H",
            "..KKKK...T.H"
        };

        for (int y = 0; y < height; y++) {
            String line = mapLines[y];
            for (int x = 0; x < width; x++) {
                char ch = (x < line.length()) ? line.charAt(x) : '.';
                Ground ground;
                switch (ch) {
                    case 'T': ground = new DustStorm(); break;
                    case 'K': ground = new CraterRock(); break;
                    case '~': ground = new MethaneLake(); break;
                    case 'I': ground = new IceCrust(); break;
                    case 'X': ground = new Obstacle(); break;
                    case 'H': ground = new DeliveryRamp(); break;
                    case 'C': ground = new MuddyPlace(); break;
                    default:  ground = new PlainGround(); break;
                }
                sampleGrid.setGround(x, y, ground);
            }
        }

        sampleGrid.addRobot(new SlidingRobot(1, new Position(0, 0), 20));
        sampleGrid.addRobot(new TrackedRobot(2, new Position(0, 2), 20));
        sampleGrid.addRobot(new FlyingRobot(3, new Position(0, 4), 20));
        sampleGrid.addRobot(new JumpingRobot(4, new Position(0, 6), 20));

        return sampleGrid;
    }
}
