package command;

import engine.Grid;
import model.common.Position;
import model.common.RobotStatus;
import model.ground.Ground;
import model.robot.Robot;

public class ForwardCommand implements Command {

    private final int steps;

    public ForwardCommand(int steps) {
        this.steps = steps;
    }

    @Override
    public boolean execute(Robot robot, Grid grid) {
        for (int i = 0; i < steps; i++) {
            
            if (!robot.isOperable()) {
                return false;
            }

            Position currentPos = robot.getPosition();
            Position nextPos = currentPos.next(robot.getDirection());

            
            if (!grid.isValidPosition(nextPos) || grid.isOccupiedByRobot(nextPos)) {
                return false; 
            }

            
            Ground targetGround = grid.getGroundAt(nextPos);

            
            boolean stepSuccess = targetGround.stepOn(robot, grid);
            if (!stepSuccess) {
                return false; 
            }


            if (robot.getPosition().equals(currentPos)) {
                robot.moveTo(nextPos);
            }

 
            if (robot.getStatus() == RobotStatus.TESLIM) {
                return true;
            }
        }
        return true;
    }
}
