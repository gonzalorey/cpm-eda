#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"../libs/defensiva.h"
#include"../libs/graphADT.h"
#include"../libs/listADT.h"

struct graphCDT
{
	stageADT source;			/*Fuente.*/
	stageADT drain;				/*Sumidero.*/
	activityADT activities;		/*Aristas.*/
	int stageNum;				/*Cantidad de nodos del grafo.*/
}; 

struct activityCDT				/*Aristas del grafo.*/
{
	actInfo * info;
	stageADT dest;
	stageADT orig;
	int li, lt,ci, ct;			/*Tiempos de la actividad.*/
	activityADT next;
};

struct stageCDT					/*Nodos del grafo.*/
{
	int tag;
	listADT finish;				/*Actividades que finalizan en esta etapa.*/
	listADT start;				/*Actividades que comienzan en esta etapa.*/
	stageADT next;				/*Nodo siguiente.*/
	stageADT before;			/*Nodo anterior.*/
};

graphADT 
NewGraph(void)
{
	graphADT rta;
	if((rta = calloc(1, sizeof(struct graphCDT))) == NULL)
		Error("No hay memoria suficiente como para crear el grafo.\n");
	if((rta->source = calloc(1, sizeof(struct stageCDT))) == NULL)
	{
		free(rta);
		Error("No hay memoria suficiente como para crear el grafo.\n");
	}
	if((rta->drain = calloc(1, sizeof(struct stageCDT))) == NULL)
	{
		free(rta->source);
		free(rta);
		Error("No hay memoria suficiente como para crear el grafo.\n");
	}
	rta->source->next = rta->drain;
	rta->drain->before = rta->source;
	rta->stageNum = 2;						/*Source y drain ya insertados.*/
	return rta;
}

/*
 * Funcion que libera los stages.
 */
static void
FreeStages(stageADT stg)
{
	if(stg)
	{
		FreeStages(stg->next);
		FreeList(&stg->start);
		FreeList(&stg->finish);
		free(stg);	
	}
}

/*
 * Funcion que libera los activities.
 */
static void
FreeActivities(activityADT act)
{
	if(act)
	{
		FreeActivities(act->next);
		free(act);	
	}
}

void 
FreeGraph(graphADT g)
{
	FreeStages(g->source);
	FreeActivities(g->activities);
	free(g);
}

stageADT
InsertStage(graphADT g)
{
	stageADT stg, aux;
	if((stg = calloc(1, sizeof(struct stageCDT))) == NULL)
	{
		FreeGraph(g);
		Error("No hay memoria suficiente como para insertar otra etapa.\n");
	}
	aux = g->drain->before;
	aux->next = stg;
	g->drain->before = stg;
	stg->start = NewList();
	stg->finish = NewList();
	stg->next = g->drain;
	stg->before = aux;
	g->stageNum++;
	return stg;
}

static int
Compare(actInfo * info1, actInfo * info2)
{
	return strcmp(info1->ID, info2->ID);	
}

static activityADT
InsertActivityWrapped(activityADT * act, actInfo * info)
{
	activityADT aux = *act;
	int cmp;
	if(!(*act) || (cmp = Compare((*act)->info, info)) > 0)
	{
		if((*act = calloc(1, sizeof(struct activityCDT))) == NULL)
		{
			*act = aux;
			return NULL;
		}
		(*act)->next = aux;
		(*act)->info = info;
		return *act;
	}
	else if(*act && cmp < 0)
		return InsertActivityWrapped(&(*act)->next, info);
	else
		return NULL;		/*Esta repedito.*/
}

activityADT
InsertActivity(graphADT g, actInfo * info, stageADT orig, stageADT dest)
{
	activityADT act;
	if((act = InsertActivityWrapped(&g->activities, info)) == NULL)
	{
		FreeGraph(g);
		Error("No hay memoria suficiente como para insertar otra actividad.\n");
	}
	act->orig = orig;
	act->dest = dest;
	if(orig)
		Insert(&orig->start, act);
	if(dest)
		Insert(&dest->finish, act);
	return act;
}

/*
 * Calcula la cantidad de actividades generadas hasta el momento.
 */
static int 
NumberOfActivitiesWrapped(activityADT act)
{
	if(!act)
		return 0;
	return 1 + NumberOfActivitiesWrapped(act->next);
}

int 
NumberOfActivities(graphADT g)
{
	return NumberOfActivitiesWrapped(g->activities);
}

int 
NumberOfStages(graphADT g)
{
	return g->stageNum;
}

/*
 * Retorna la actividad que se corresponde con tal ID. Si no existe,
 * devuelve NULL.
 */
static activityADT
GetActivityWrapped(activityADT act, char * ID)
{
	int cmp;
	if(act && (cmp = strcmp(act->info->ID, ID)) < 0)
		return GetActivityWrapped(act->next, ID);
	else if(act && !cmp)
		return act;
	else
		return NULL;		/*Si es mayor o si act == NULL, el elemento no esta.*/
}

activityADT
GetActivity(graphADT g, char * ID)
{
	return GetActivityWrapped(g->activities, ID);
}

int 
ElementBelongsToGraph(graphADT g, char * ID)
{
	return (GetActivity(g, ID) != NULL);
}

int
IsSource(graphADT g, stageADT stg)
{
	return (stg == g->source);	
}

int
IsDrain(graphADT g, stageADT stg)
{
	return (stg == g->drain);	
}

stageADT
GetSource(graphADT g)
{
	return g->source;	
}

stageADT
GetDrain(graphADT g)
{
	return g->drain;
}

int
SetActivityOrig(graphADT g, char * ID, stageADT stg)
{
	activityADT aux;
	if((aux = GetActivity(g, ID)) == NULL)
		return 0;
	if(aux->orig)							/*Si ya tenia un origen, la borro del mismo.*/
		Delete(&aux->orig->start, ID);
	aux->orig = stg;
	Insert(&stg->start, aux);
	return 1;
}

int
SetActivityDest(graphADT g, char * ID, stageADT stg)
{
	activityADT aux;
	if((aux = GetActivity(g, ID)) == NULL)
		return 0;
	if(aux->dest)							/*Si ya tenia un destino, la borro del mismo.*/
		Delete(&aux->dest->finish, ID);
	aux->dest = stg;
	Insert(&stg->finish, aux);
	return 1;
}

stageADT
GetActivityOrig(graphADT g, char * ID)
{
	activityADT aux;
	if((aux = GetActivity(g, ID)) == NULL)
		return NULL;
	return aux->orig;
}

stageADT
GetActivityDest(graphADT g, char * ID)
{
	activityADT aux;
	if((aux = GetActivity(g, ID)) == NULL)
		return NULL;
	return aux->dest;
}

/*
 * Borra los origenes de todas las actividades dentro de la lista.
 */
static void
DeleteActivitiesOrig(listADT list)
{
	activityADT act;
	if(!ListIsEmpty(list))
	{
		act = ListHead(list);
		act->orig = NULL;
		DeleteActivitiesOrig(ListTail(list));
	}	
}

/*
 * Borra los destinos de todas las actividades dentro de la lista.
 */
static void
DeleteActivitiesDest(listADT list)
{
	activityADT act;
	if(!ListIsEmpty(list))
	{
		act = ListHead(list);
		act->dest = NULL;
		DeleteActivitiesDest(ListTail(list));
	}	
}

int
DeleteStage(graphADT g, stageADT stg)
{
	if(IsSource(g, stg) || IsDrain(g, stg))
		return 0;					/*Ni la fuente ni el sumidero pueden ser borrados.*/
	stg->before->next = stg->next;
	stg->next->before = stg->before;
	DeleteActivitiesOrig(stg->start);
	DeleteActivitiesDest(stg->finish);
	FreeList(&stg->start);
	FreeList(&stg->finish);
	free(stg);
	return 1;
}

int
DeleteActivity(graphADT g, char * ID)
{
	activityADT act, aux;
	if((act = GetActivity(g, ID)) == NULL)
		return 0;
	if(act->orig)					/*La borro en las etapas.*/
		Delete(&act->orig->finish, ID);	
	if(act->dest)
		Delete(&act->dest->start, ID);
	aux = act;
	act = act->next;
	free(aux);
	return 1;
}
