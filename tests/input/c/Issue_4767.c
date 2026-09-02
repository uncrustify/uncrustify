typedef struct IMUIShell
{
	uint32_t (*AddRef)(IMUIObject *Self);
	uint32_t (*Release)(IMUIObject *Self);
	uint32_t (*Version)(IMUIObject *Self);
	uint32_t (*IfaceId)(IMUIObject *Self);
	IMUIObject *(*QueryIface)(IMUIObject *Self, uint32_t IFaceId);
} IMUIShell;
