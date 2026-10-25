#ifndef OH_CORE_TYPES_H
#define OH_CORE_TYPES_H

// Protocol V133 — shared codes (openGo legacy.go / client contract).
// Domains use these; they do not invent parallel enums.

namespace oh {

// Search / list channels (client + search endpoints).
enum Channel {
    ChannelAll       = -10,
    ChannelVacancies = -5,
    ChannelNews      = -20, // client-only mark for news rows in the list
    ChannelCamp      = 0,
    ChannelShow      = 1,
    ChannelPere      = 2,
    ChannelTele      = 3
};

// Detail request kind (what fetchDetail loads).
enum DetailKind {
    DetailProject      = 0,
    DetailVacancy      = 1,
    DetailNews         = 2,
    DetailProjectNews  = 3,
    DetailProfile      = 4,
    DetailProfileWorks = 5,
    DetailProfileAdded = 6,
    DetailCommentsPage = 7
};

// Like API "type" query (LikeTypes in openGo). Item vs comment.
inline int likeTypeProject()  { return 0; }
inline int likeTypeNews()     { return 2; }
inline int likeTypeVacancy()  { return 11; }
inline int likeTypeCommentProject()  { return 3; }
inline int likeTypeCommentNews()     { return 5; }
inline int likeTypeCommentVacancy()  { return 12; }

// Comment content "type" for fetchComms / comment send (DB channel).
inline int commentTypeProject()  { return 0; }
inline int commentTypeNews()     { return 3; }
inline int commentTypeVacancy()  { return 5; }

inline int commentTypeForDetail(int detailKind)
{
    switch (detailKind) {
    case DetailProject: return commentTypeProject();
    case DetailNews:    return commentTypeNews();
    case DetailVacancy: return commentTypeVacancy();
    default:            return -1;
    }
}

inline int itemLikeTypeForDetail(int detailKind)
{
    switch (detailKind) {
    case DetailProject: return likeTypeProject();
    case DetailNews:    return likeTypeNews();
    case DetailVacancy: return likeTypeVacancy();
    default:            return -1;
    }
}

inline int commentLikeTypeForDetail(int detailKind)
{
    switch (detailKind) {
    case DetailProject: return likeTypeCommentProject();
    case DetailNews:    return likeTypeCommentNews();
    case DetailVacancy: return likeTypeCommentVacancy();
    default:            return -1;
    }
}

// likesT.php category letters.
inline const char *likeCategoryLetter(int typeCode)
{
    switch (typeCode) {
    case 0:  return "p";
    case 2:  return "n";
    case 11: return "v";
    case 3:
    case 5:
    case 12: return "c";
    default: return "";
    }
}

} // namespace oh

#endif
