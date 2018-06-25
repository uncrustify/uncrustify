class BSRRE1D_file : PhysicalFile
{
	int getFoo() { return(m_foo); }

	void setFoo(int foo) { m_foo = foo; }

	public BSRRE1D_file() {
		this.addFormatName("BSRRE1DF");
	}



	private int m_foo;
	public void xxx() {
		ahoj();
	} // comment



	public void yyy() {
		ahoj();
	}



	/* comment 2 */
	public void xxx() {
		ahoj();
	}
}
