package command;

import engine.Grid;
import model.robot.Robot;

public class LeftCommand implements Command{

	@Override
	public boolean execute(Robot robot, Grid grid) {
		if(!robot.isOperable()) return false;
		robot.turnLeft();
		return true;
	}

}
