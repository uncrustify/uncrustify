template < typename TImage >
class MorphologicalContourInterpolator :
	public ImageToImageFilter< TImage, TImage >
{
template < typename T >
friend class MorphologicalContourInterpolatorParallelInvoker;
friend class ::MultiLabelMeshPipeline;

public:
/** Standard class typedefs. */
typedef MorphologicalContourInterpolator Self;

protected:
MorphologicalContourInterpolator();
~MorphologicalContourInterpolator() {
}
typename TImage::PixelType m_Label;
int                        m_Axis;
bool                       m_HeuristicAlignment;

private:
MorphologicalContourInterpolator( const Self& ) ITK_DELETE_FUNCTION;
void
operator=( const Self& ) ITK_DELETE_FUNCTION;
};
