package engine;

import model.common.Direction;
import model.common.Position;
import model.common.RobotStatus;
import model.robot.Robot;

import java.util.ArrayList;
import java.util.List;

public class SimulationState {

    public static class RobotSnapshot {
        private final int id;
        private final Position position;
        private final Direction direction;
        private final int energy;
        private final RobotStatus status;

        public RobotSnapshot(Robot robot) {
            this.id = robot.getId();
            this.position = new Position(robot.getPosition().getX(), robot.getPosition().getY());
            this.direction = robot.getDirection();
            this.energy = robot.getEnergy();
            this.status = robot.getStatus();
        }

        public void restoreTo(Robot robot) {
            if (robot.getId() != this.id) return;
            robot.restoreState(this.position, this.direction, this.energy, this.status);
        }

        public int getId() { return id; }
        public Position getPosition() { return position; }
        public Direction getDirection() { return direction; }
        public int getEnergy() { return energy; }
        public RobotStatus getStatus() { return status; }
    }

    private final List<RobotSnapshot> robotSnapshots = new ArrayList<>();
    private final int currentCommandIndex;
    private final int currentRobotIndex;
    private final String description;

    public SimulationState(List<Robot> robots, int cmdIndex, int robotIndex, String desc) {
        for (Robot r : robots) {
            robotSnapshots.add(new RobotSnapshot(r));
        }
        this.currentCommandIndex = cmdIndex;
        this.currentRobotIndex = robotIndex;
        this.description = desc;
    }

    public List<RobotSnapshot> getRobotSnapshots() {
        return robotSnapshots;
    }

    public int getCurrentCommandIndex() {
        return currentCommandIndex;
    }

    public int getCurrentRobotIndex() {
        return currentRobotIndex;
    }

    public String getDescription() {
        return description;
    }
}
