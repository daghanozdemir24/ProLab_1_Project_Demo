package model.common;

public enum Direction {
	N(0,-1),
	S(0,1),
	W(-1,0),
	E(1,0);
	
	private final int dx;
	private final int dy;
	
	Direction(int dx, int dy) {
        this.dx = dx;
        this.dy = dy;
    }
	
	public int getDx() {
		return dx;
	}
	public int getDy() {
		return dy;
	}
	
	public Direction turnRight() {
		switch (this) {
		
		case N: return E;
		case E: return S;
		case S: return W;
		case W: return N;
		default:
			return this;
		}
	}
		
	public Direction turnLeft() {
			switch (this) {
			
			case N: return W;
			case E: return N;
			case S: return E;
			case W: return S;
			default:
				return this;
			}
		
		
		
	}
	
	
	
}

