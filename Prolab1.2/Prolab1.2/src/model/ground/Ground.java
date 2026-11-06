package model.ground;

import engine.Grid;
import model.robot.Robot;

public abstract class Ground {

	private final char symbol;

	public Ground(char symbol) {
		this.symbol = symbol;
	}

	public char getSymbol() {
		return symbol;
	}
	
	public abstract boolean stepOn(Robot robot, Grid grid);
	
}

