package command;

import engine.Grid;
import model.robot.Robot;

public interface Command {

	boolean execute(Robot robot, Grid grid);

}
