package engine;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import model.common.Position;
import model.common.RobotStatus;
import model.ground.Ground;
import model.robot.Robot;

public class Grid {
    private final int width;
    private final int height;
    private final Ground[][] cells;
    private final List<Robot> robots;

    public Grid(int width, int height) {
        this.width = width;
        this.height = height;
        this.cells = new Ground[height][width]; // [satır (y)][sütun (x)]
        this.robots = new ArrayList<>();
    }

    // Belirli bir koordinata zemin yerleştirir (Dosya okunurken çağrılır)
    public void setGround(int x, int y, Ground ground) {
        if (isValidCoordinate(x, y)) {
            this.cells[y][x] = ground;
        }
    }

    // Belirli bir konumdaki zemini döner
    public Ground getGroundAt(Position pos) {
        if (!isValidPosition(pos)) {
            return null;
        }
        return this.cells[pos.getY()][pos.getX()];
    }

    // Haritaya robot ekler
    public void addRobot(Robot robot) {
        this.robots.add(robot);
    }

    // Bir konum harita sınırları içinde mi?
    public boolean isValidPosition(Position pos) {
        if (pos == null) return false;
        return isValidCoordinate(pos.getX(), pos.getY());
    }

    private boolean isValidCoordinate(int x, int y) {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    // O karede AKTIF veya ENERJISIZ başka bir robot var mı?
    // Not: Şartnameye göre TESLIM olan robot haritadan kalktığı için engel sayılmaz!
    public boolean isOccupiedByRobot(Position pos) {
        for (Robot r : robots) {
            if (r.getStatus() != RobotStatus.TESLIM && r.getPosition().equals(pos)) {
                return true;
            }
        }
        return false;
    }

    // Belirli bir konumdaki robotu bulur (varsa)
    public Robot getRobotAt(Position pos) {
        for (Robot r : robots) {
            if (r.getStatus() != RobotStatus.TESLIM && r.getPosition().equals(pos)) {
                return r;
            }
        }
        return null;
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public List<Robot> getRobots() {
        return Collections.unmodifiableList(robots);
    }
}

